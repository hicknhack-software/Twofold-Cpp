import * as path from 'path';
import * as child_process from 'child_process';
import * as fs from 'fs';
import type * as vscodeTypes from 'vscode';

// Dynamic import for vscode to allow running outside VS Code (e.g., for testing)
let vscodeModule: typeof vscodeTypes;
async function getVscode() {
	if (!vscodeModule) {
		vscodeModule = await import('vscode');
	}
	return vscodeModule;
}

interface LineClassification {
	indent: string;
	directive: string;
	content: string;
	originalLine: string;
}

export class TwofoldFormatter {
	private clangFormatPath: string | null = null;
	private targetColumn: number | null = null;
	private inferredColumn: number | null = null;
	private preservedLines: Array<{ lineNumber: number; originalLine: string }> = [];

	constructor(clangFormatPath: string | null, targetColumn: number | null) {
		this.clangFormatPath = clangFormatPath;
		this.targetColumn = targetColumn;
	}

	private static readonly OUTPUT_LINE_PATTERN = /^(\s*)([|\\])(.*)$/;
	private static readonly INDENT_DIRECTIVE_PATTERN = /^(\s*)(=)(.*)$/;
	// Placeholder for preserved lines during clang-format processing
	private static readonly PRESERVED_LINE_PLACEHOLDER = 'TWOFOLD_PRESERVED_LINE';

	public static async create(targetColumn: number | null): Promise<TwofoldFormatter> {
		const vscodeModule = await getVscode();
		const config = vscodeModule.workspace.getConfiguration('twofold');
		const clangFormatPath = config.get<string>('clangFormatPath') ?? null;

		const formatter = new TwofoldFormatter(clangFormatPath, targetColumn);
		await formatter.findClangFormat();
		return formatter;
	}

	public static async createWithPath(clangFormatPath: string, targetColumn: number | null): Promise<TwofoldFormatter> {
		const formatter = new TwofoldFormatter(clangFormatPath, targetColumn);
		// Skip findClangFormat if path is already provided (for testing)
		return formatter;
	}

	private async findClangFormat(): Promise<void> {
		if (this.clangFormatPath) {
			return;
		}

		// Try C_Cpp.clang_format_path first
		const vscodeModule = await getVscode();
		const cCppConfig = vscodeModule.workspace.getConfiguration('C_Cpp');
		const cCppClangFormat = cCppConfig.get<string>('clang_format_path');

		if (cCppClangFormat && cCppClangFormat.length > 0) {
			this.clangFormatPath = cCppClangFormat;
			return;
		}

		// Fall back to PATH search
		const clangFormat = await TwofoldFormatter.findInPath('clang-format');
		if (clangFormat) {
			this.clangFormatPath = clangFormat;
			return;
		}

		throw new Error('clang-format not found. Please set C_Cpp.clang_format_path or twofold.clangFormatPath.');
	}

	private static findInPath(command: string): Promise<string | null> {
		return new Promise((resolve) => {
			const isWindows = process.platform === 'win32';
			const cmd = isWindows ? `where ${command}` : `which ${command}`;

			child_process.exec(cmd, (error, stdout) => {
				if (error || !stdout.trim()) {
					resolve(null);
					return;
				}
				const exePath = stdout.trim().split('\n')[0];
				resolve(exePath);
			});
		});
	}

	private inferTargetColumn(lines: string[]): number | null {
		for (const line of lines) {
			const match = line.match(TwofoldFormatter.OUTPUT_LINE_PATTERN);
			if (match) {
				return match[1].length;
			}
		}
		return null;
	}

	private classifyLine(line: string): LineClassification {
		// Check for output directive (| or \)
		let match = line.match(TwofoldFormatter.OUTPUT_LINE_PATTERN);
		if (match) {
			const [, indent, directive, content] = match;
			// Check if this is actually an escape sequence \#{ or \#
			if (directive === '\\' && content.startsWith('#')) {
				return { indent: '', directive: '', content: '', originalLine: line };
			}
			return { indent, directive, content, originalLine: line };
		}

		// Check for indent directive (=)
		match = line.match(TwofoldFormatter.INDENT_DIRECTIVE_PATTERN);
		if (match) {
			const [, indent, directive, content] = match;
			return { indent, directive, content, originalLine: line };
		}

		// Host code line
		return { indent: '', directive: '', content: '', originalLine: line };
	}

	private convertToCpp(lines: string[]): string[] {
		const converted: string[] = [];
		this.preservedLines = [];

		for (let lineNum = 0; lineNum < lines.length; lineNum++) {
			const line = lines[lineNum];
			const { indent, directive, content, originalLine } = this.classifyLine(line);

			if (directive === '|' || directive === '\\' || directive === '=') {
				// Store the original line for later restoration
				this.preservedLines.push({ lineNumber: converted.length, originalLine });
				// Replace with a placeholder
				converted.push(TwofoldFormatter.PRESERVED_LINE_PLACEHOLDER);
			} else {
				// Keep as-is (host code)
				converted.push(line);
			}
		}

		return converted;
	}

	private runClangFormat(content: string): Promise<string> {
		return new Promise((resolve, reject) => {
			if (!this.clangFormatPath) {
				reject(new Error('clang-format path not set'));
				return;
			}

			// Use ColumnLimit: 0 to prevent line wrapping (keeps raw strings on single lines)
			const child = child_process.spawn(this.clangFormatPath, ['-style={ColumnLimit: 0}'], {
				stdio: ['pipe', 'pipe', 'pipe']
			});

			let stdout = '';
			let stderr = '';

			child.stdout.on('data', (data) => {
				stdout += data.toString();
			});

			child.stderr.on('data', (data) => {
				stderr += data.toString();
			});

			child.on('close', (code) => {
				if (code === 0) {
					resolve(stdout);
				} else {
					reject(new Error(`clang-format failed: ${stderr}`));
				}
			});

			child.on('error', (err) => {
				reject(new Error(`Failed to run clang-format: ${err.message}`));
			});

			child.stdin.write(content);
			child.stdin.end();
		});
	}

	private restorePreservedLines(lines: string[]): string[] {
		const result = [...lines];

		// Replace placeholder lines with original content
		for (const { lineNumber, originalLine } of this.preservedLines) {
			if (lineNumber < result.length) {
				result[lineNumber] = originalLine;
			}
		}

		return result;
	}

	private realignOutputLines(lines: string[], targetColumn: number): string[] {
		// Only realign if targetColumn > 0 (explicit setting)
		// If targetColumn is 0 (inferred), preserve clang-format's indentation
		if (targetColumn <= 0) {
			return lines;
		}

		const result: string[] = [];

		for (const line of lines) {
			// Check for output directive (| or \)
			let match = line.match(TwofoldFormatter.OUTPUT_LINE_PATTERN);
			if (match) {
				const [, indent, directive, content] = match;
				const newIndent = ' '.repeat(targetColumn);
				result.push(`${newIndent}${directive}${content}`);
				continue;
			}

			// Check for indent directive (=)
			match = line.match(TwofoldFormatter.INDENT_DIRECTIVE_PATTERN);
			if (match) {
				const [, indent, directive, content] = match;
				const newIndent = ' '.repeat(targetColumn);
				result.push(`${newIndent}${directive}${content}`);
				continue;
			}

			result.push(line);
		}

		return result;
	}

	public async formatContent(content: string): Promise<string> {
		// Handle both Unix and Windows line endings
		// Match Python's splitlines() behavior: exclude trailing empty string when content ends with newline
		const normalized = content.replace(/\r\n/g, '\n');
		const hasTrailingNewline = normalized.endsWith('\n');
		// Remove trailing empty string if content originally ended with newline
		const lines = normalized.split('\n');
		if (hasTrailingNewline && lines[lines.length - 1] === '') {
			lines.pop();
		}

		// Determine target column
		let effectiveColumn = this.targetColumn;
		if (effectiveColumn === null || effectiveColumn === undefined) {
			effectiveColumn = this.inferTargetColumn(lines) ?? 0;
		}

		this.inferredColumn = effectiveColumn;

		// Step 1: Convert to formatable C++
		const converted = this.convertToCpp(lines);

		// Step 2: Run clang-format
		const cppContent = converted.join('\n');
		const formattedCpp = await this.runClangFormat(cppContent);

		// Step 3: Restore preserved lines (output and indent directives)
		let formattedLines = formattedCpp.split('\n');
		let restored = this.restorePreservedLines(formattedLines);

		// Step 4: Realign output lines to target column
		restored = this.realignOutputLines(restored, effectiveColumn);

		return restored.join('\n') + '\n';
	}

	public async formatFile(filePath: string, dryRun: boolean = false): Promise<boolean> {
		const content = fs.readFileSync(filePath, 'utf-8');
		const formatted = await this.formatContent(content);

		const changed = content !== formatted;

		if (!dryRun && changed) {
			fs.writeFileSync(filePath, formatted, 'utf-8');
		}

		return changed;
	}
}

export class TwicefoldDocumentFormatter implements vscodeTypes.DocumentFormattingEditProvider {
	public async provideDocumentFormattingEdits(
		document: vscodeTypes.TextDocument,
		options: vscodeTypes.FormattingOptions,
		token: vscodeTypes.CancellationToken
	): Promise<vscodeTypes.TextEdit[]> {
		const vscodeModule = await getVscode();
		const config = vscodeModule.workspace.getConfiguration('twofold');
		const targetColumn = config.get<number>('targetColumn') ?? 32;

		const formatter = await TwofoldFormatter.create(targetColumn);

		const content = document.getText();
		const formatted = await formatter.formatContent(content);

		const fullRange = new vscodeModule.Range(
			document.positionAt(0),
			document.positionAt(content.length)
		);

		return [vscodeModule.TextEdit.replace(fullRange, formatted)];
	}
}
