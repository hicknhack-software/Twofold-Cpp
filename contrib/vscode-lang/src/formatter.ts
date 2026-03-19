import * as path from 'path';
import * as child_process from 'child_process';
import * as fs from 'fs';
import type * as vscodeTypes from 'vscode';
import { TextEncoder } from 'util';

// Dynamic import for vscode to allow running outside VS Code (e.g., for testing)
let vscodeModule: typeof vscodeTypes | null = null;
let vscodeModuleError: Error | null = null;

async function getVscode(): Promise<typeof vscodeTypes | null> {
	if (vscodeModuleError) {
		return null;
	}
	if (vscodeModule) {
		return vscodeModule;
	}
	try {
		vscodeModule = await import('vscode');
		return vscodeModule;
	} catch (error) {
		vscodeModuleError = error as Error;
		return null;
	}
}

interface LineClassification {
	indent: string;
	directive: string;
	content: string;
	originalLine: string;
}

export class TwofoldFormatter {
	private targetColumn: number | null = null;
	private inferredColumn: number | null = null;
	private preservedLines: Array<{ lineNumber: number; originalLine: string }> = [];
	private workspaceFolder: vscodeTypes.WorkspaceFolder | null = null;
	private clangFormatPath: string | null = null; // Fallback for CLI testing

	constructor(targetColumn: number | null, workspaceFolder: vscodeTypes.WorkspaceFolder | null = null, clangFormatPath: string | null = null) {
		this.targetColumn = targetColumn;
		this.workspaceFolder = workspaceFolder;
		this.clangFormatPath = clangFormatPath;
	}

	private static readonly OUTPUT_LINE_PATTERN = /^(\s*)([|\\])(.*)$/;
	private static readonly INDENT_DIRECTIVE_PATTERN = /^(\s*)(=)(.*)$/;

	public static async create(targetColumn: number | null): Promise<TwofoldFormatter> {
		const vscode = await getVscode();
		if (!vscode) {
			// VS Code not available (shouldn't happen in extension context)
			return new TwofoldFormatter(targetColumn, null, null);
		}

		const config = vscode.workspace.getConfiguration('twofold');

		// Get the active workspace folder
		const workspaceFolder = vscode.workspace.workspaceFolders?.[0] ?? null;

		return new TwofoldFormatter(targetColumn, workspaceFolder, null);
	}

	public static async createWithPath(clangFormatPath: string, targetColumn: number | null): Promise<TwofoldFormatter> {
		// If we have a clang-format path, we can skip getting workspace (for CLI testing)
		if (clangFormatPath) {
			return new TwofoldFormatter(targetColumn, null, clangFormatPath);
		}
		// Otherwise, try to get workspace folder
		const vscode = await getVscode();
		const workspaceFolder = vscode?.workspace.workspaceFolders?.[0] ?? null;
		return new TwofoldFormatter(targetColumn, workspaceFolder, clangFormatPath);
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
				const lineNumber = converted.length;
				this.preservedLines.push({ lineNumber, originalLine });
				// Replace with a unique placeholder (add semicolon to make valid C++)
				converted.push(`TWOFOLD_LINE_${lineNumber};`);
			} else {
				// Keep as-is (host code)
				converted.push(line);
			}
		}

		return converted;
	}

	private async runVsCodeFormat(content: string): Promise<string> {
		// If we have a workspace folder and VS Code is available, use VS Code's formatting
		if (this.workspaceFolder) {
			const vscode = await getVscode();
			if (vscode !== null) {
				return this.runVsCodeFormatWithWorkspace(vscode, content);
			}
		}

		// Fall back to direct clang-format if path is provided (for CLI testing)
		if (this.clangFormatPath) {
			return this.runClangFormatDirect(content);
		}

		console.warn('No workspace folder or clang-format path available, skipping formatting');
		return content;
	}

	private async runVsCodeFormatWithWorkspace(vscode: typeof vscodeTypes, content: string): Promise<string> {
		// Create a temporary file in the workspace
		const tempFileName = `.temp_format_${Date.now()}.cpp`;
		const tempFileUri = vscode.Uri.joinPath(this.workspaceFolder!.uri, tempFileName);

		try {
			// Write the unformatted code to a physical file
			const contentBytes = new TextEncoder().encode(content);
			await vscode.workspace.fs.writeFile(tempFileUri, contentBytes);

			// Open the document in VS Code's memory
			const document = await vscode.workspace.openTextDocument(tempFileUri);

			// Request formatting edits from VS Code's standard formatter API
			// This delegates to the C/C++ extension (ms-vscode.cpptools) which uses clang-format
			const edits = await vscode.commands.executeCommand<vscodeTypes.TextEdit[]>(
				'vscode.executeFormatDocumentProvider',
				document.uri,
				{ tabSize: 4, insertSpaces: true }
			);

			// If no edits, return original content
			if (!edits || edits.length === 0) {
				return content;
			}

			// Apply edits manually to get the formatted string
			// Sort in reverse order (bottom-to-top) so edits don't shift offsets
			const sortedEdits = edits.sort((a, b) =>
				b.range.start.compareTo(a.range.start)
			);

			let formattedCode = document.getText();
			for (const edit of sortedEdits) {
				const startOffset = document.offsetAt(edit.range.start);
				const endOffset = document.offsetAt(edit.range.end);
				formattedCode =
					formattedCode.substring(0, startOffset) +
					edit.newText +
					formattedCode.substring(endOffset);
			}

			return formattedCode;
		} catch (error) {
			console.error('VS Code formatting failed:', error);
			// Fall back to returning original content on error
			return content;
		} finally {
			// Clean up the temporary file
			try {
				await vscode.workspace.fs.delete(tempFileUri);
			} catch {
				// Ignore cleanup errors
			}
		}
	}

	private runClangFormatDirect(content: string): Promise<string> {
		return new Promise((resolve, reject) => {
			// Use ColumnLimit: 0 to prevent line wrapping (keeps raw strings on single lines)
			const child = child_process.spawn(this.clangFormatPath!, ['-style={ColumnLimit: 0}'], {
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
		const result: string[] = [];

		// Build a map of markers to original lines
		const markerToOriginal = new Map<number, string>();
		for (const { lineNumber, originalLine } of this.preservedLines) {
			markerToOriginal.set(lineNumber, originalLine);
		}

		for (let line of lines) {
			let searchAgain = true;
			while (searchAgain) {
				searchAgain = false;
				for (const [lineNumber, originalLine] of markerToOriginal) {
					// Regex to find marker, allowing for variations in whitespace around semicolon
					const markerRegex = new RegExp(`TWOFOLD_LINE_${lineNumber}\\s*;`);
					const match = line.match(markerRegex);
					
					if (match) {
						const index = match.index!;
						const markerLength = match[0].length;
						const before = line.substring(0, index);
						const after = line.substring(index + markerLength);
						
						// If there was code before the marker on the same line, push it
						if (before.trim().length > 0) {
							result.push(before);
						}
						
						// Push the original twofold line
						result.push(originalLine);
						
						// Continue searching in the 'after' part
						line = after;
						searchAgain = true;
						break;
					}
				}
			}
			// If there's anything left on the line (like merged C++ code), push it
			if (line.trim().length > 0) {
				result.push(line);
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

		// Step 2: Run VS Code formatting (delegates to C/C++ extension/clang-format)
		const cppContent = converted.join('\n');
		const formattedCpp = await this.runVsCodeFormat(cppContent);

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
		const vscode = await getVscode();
		if (!vscode) {
			// Shouldn't happen in VS Code context, but handle gracefully
			return [];
		}

		const config = vscode.workspace.getConfiguration('twofold');
		const targetColumn = config.get<number>('targetColumn') ?? 32;

		// Get the workspace folder from the document
		const workspaceFolder = vscode.workspace.getWorkspaceFolder(document.uri) ?? null;

		const formatter = new TwofoldFormatter(targetColumn, workspaceFolder, null);

		const content = document.getText();
		const formatted = await formatter.formatContent(content);

		const fullRange = new vscode.Range(
			document.positionAt(0),
			document.positionAt(content.length)
		);

		return [vscode.TextEdit.replace(fullRange, formatted)];
	}
}
