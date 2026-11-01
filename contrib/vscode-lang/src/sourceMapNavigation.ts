// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

import * as path from 'path';
import * as vscode from 'vscode';
import { SourcePosition, Trace } from './sourceMap';
import { MappedDocument, SourceMapService } from './sourceMapService';
import { commandLink, configurationLink, configureSourceMapping, SourceReference } from './sourceMapConfiguration';

interface NavigationTarget { mapped: MappedDocument; position: SourcePosition; trace?: Trace; uri?: vscode.Uri; }
interface OpenReference extends SourceReference { line: number; column: number; uri?: string; generatedLine?: number; generatedColumn?: number; }

class SourceItem extends vscode.TreeItem {
	constructor(label: string, readonly children?: SourceItem[], readonly target?: NavigationTarget) {
		super(label, children ? vscode.TreeItemCollapsibleState.Expanded : vscode.TreeItemCollapsibleState.None);
		if (target) {
			this.command = { command: 'twofold.openSource', title: 'Open Generating Source', arguments: [target] };
			const tooltip = new vscode.MarkdownString();
			tooltip.appendText(`${target.mapped.map.sources[target.position.source]}:${target.position.line + 1}:${target.position.column + 1}`);
			tooltip.appendMarkdown(`\n\n[Configure source mapping…](${configurationLink(target.mapped, target.position.source)})`);
			tooltip.isTrusted = { enabledCommands: ['twofold.configureSourceMap'] };
			this.tooltip = tooltip;
			this.iconPath = new vscode.ThemeIcon('go-to-file');
		}
	}
}

function locationLabel(mapped: MappedDocument, position: SourcePosition): string {
	return `${mapped.map.sources[position.source]}:${position.line + 1}:${position.column + 1}`;
}

class SourceTree implements vscode.TreeDataProvider<SourceItem>, vscode.Disposable {
	private readonly changed = new vscode.EventEmitter<SourceItem | undefined>();
	readonly onDidChangeTreeData = this.changed.event;
	private items: SourceItem[] = [];
	constructor(private readonly service: SourceMapService) {}

	set(mapped?: MappedDocument, trace?: Trace): void {
		this.items = mapped ? [
			new SourceItem('Generating Source', trace
				? [new SourceItem(locationLabel(mapped, trace.original), undefined, { mapped, position: trace.original, trace })]
				: [new SourceItem('No mapping at the cursor')]),
			new SourceItem('Stack Trace', trace?.stack.length
				? trace.stack.map((position, index) => new SourceItem(`${index + 1}. ${locationLabel(mapped, position)}`, undefined, { mapped, position }))
				: [new SourceItem('No caller stack at the cursor')]),
			new SourceItem(`Sources (${mapped.map.sources.length})`, mapped.map.sources.map((source, index) => {
				const position = trace?.original.source === index ? trace.original : mapped.map.firstPosition(index);
				return new SourceItem(source, undefined, { mapped, position, trace: trace?.original.source === index ? trace : undefined });
			}))
		] : [];
		this.changed.fire(undefined);
	}

	async getTreeItem(item: SourceItem): Promise<vscode.TreeItem> {
		if (item.target && !item.target.uri) {
			try {
				const matches = await this.service.sourceUris(item.target.mapped, item.target.position.source);
				item.collapsibleState = matches.length > 1 ? vscode.TreeItemCollapsibleState.Collapsed : vscode.TreeItemCollapsibleState.None;
				item.description = matches.length > 1 ? `${matches.length} matches` : undefined;
			} catch { item.description = 'Source not found'; item.collapsibleState = vscode.TreeItemCollapsibleState.None; }
		}
		return item;
	}
	async getChildren(item?: SourceItem): Promise<SourceItem[]> {
		if (item?.target && !item.target.uri) {
			try {
				const matches = await this.service.sourceUris(item.target.mapped, item.target.position.source);
				return matches.length > 1 ? matches.map(uri => new SourceItem(vscode.workspace.asRelativePath(uri, true), undefined,
					{ ...item.target!, uri })) : [];
			} catch { return []; }
		}
		return item?.children ?? (item ? [] : this.items);
	}
	dispose(): void { this.changed.dispose(); }
}

export function registerSourceMapNavigation(context: vscode.ExtensionContext): void {
	const service = new SourceMapService();
	const tree = new SourceTree(service);
	const view = vscode.window.createTreeView('twofold.sources', { treeDataProvider: tree, showCollapseAll: true });
	const highlight = vscode.window.createTextEditorDecorationType({
		backgroundColor: new vscode.ThemeColor('editor.findMatchHighlightBackground'),
		borderColor: new vscode.ThemeColor('editor.findMatchHighlightBorder'), borderWidth: '1px', borderStyle: 'solid'
	});
	const output = vscode.window.createOutputChannel('Twofold Source Maps');
	let mapped: MappedDocument | undefined;
	let trace: Trace | undefined;
	let request = 0;
	let timer: ReturnType<typeof setTimeout> | undefined;
	const sourceDocuments = new Set<string>();
	const clearHighlights = () => vscode.window.visibleTextEditors.forEach(editor => editor.setDecorations(highlight, []));
	const report = (error: unknown): string => error instanceof Error ? error.message : String(error);

	const update = async (editor = vscode.window.activeTextEditor) => {
		const currentRequest = ++request;
		if (!editor) { return; }
		try {
			const next = await service.load(editor.document);
			if (currentRequest !== request) { return; }
			if (!next && mapped && sourceDocuments.has(editor.document.uri.toString()) && editor.document.uri.toString() !== mapped.document.uri.toString()) {
				return; // Keep the output's stack visible, while allowing sources with their own map to take over.
			}
			mapped = next;
			trace = next ? service.trace(next, editor.selection.active) : undefined;
			tree.set(mapped, trace);
			view.description = mapped ? path.posix.basename(mapped.document.uri.path) : undefined;
			view.message = mapped ? undefined : 'Open a generated file containing a sourceMappingURL comment.';
		} catch (error) {
			if (currentRequest !== request) { return; }
			mapped = undefined; trace = undefined; tree.set();
			view.description = undefined;
			view.message = `Cannot load source map: ${report(error)}`;
		}
	};
	const schedule = () => {
		if (timer) { clearTimeout(timer); }
		timer = setTimeout(() => { timer = undefined; void update(); }, 100);
	};
	const open = async (target: NavigationTarget | undefined) => {
		if (!target) { return; }
		try {
			let uri = target.uri;
			if (!uri) {
				const matches = await service.sourceUris(target.mapped, target.position.source);
				uri = matches.length === 1 ? matches[0] : (await vscode.window.showQuickPick(matches.map(uri => ({
					label: vscode.workspace.asRelativePath(uri, true), description: uri.fsPath, uri
				})), { title: 'Choose a generating source', matchOnDescription: true }))?.uri;
			}
			if (!uri) { return; }
			const source = await service.target(target.mapped, target.position, target.trace, uri);
			sourceDocuments.add(source.document.uri.toString());
			const editor = await vscode.window.showTextDocument(source.document, { preview: true, selection: source.selection });
			clearHighlights();
			editor.setDecorations(highlight, [source.section]);
			editor.revealRange(source.section, vscode.TextEditorRevealType.InCenterIfOutsideViewport);
		} catch (error) {
			void vscode.window.showErrorMessage(`Twofold: ${report(error)}`);
		}
	};
	const fromReference = async (reference: SourceReference): Promise<MappedDocument | undefined> => {
		if (!reference || typeof reference.generated !== 'string' || !Number.isInteger(reference.source)) { return; }
		const current = await service.load(await vscode.workspace.openTextDocument(vscode.Uri.parse(reference.generated)));
		return current && reference.source >= 0 && reference.source < current.map.sources.length ? current : undefined;
	};

	context.subscriptions.push(service, tree, view, highlight, output,
		{ dispose: () => { if (timer) { clearTimeout(timer); } } },
		vscode.commands.registerCommand('twofold.openSource', open),
		vscode.commands.registerCommand('twofold.openMappedSource', async (reference: OpenReference) => {
			try {
				const current = await fromReference(reference);
				if (!current || !Number.isInteger(reference.line) || reference.line < 0 || !Number.isInteger(reference.column) || reference.column < 0) { return; }
				const matches = await service.sourceUris(current, reference.source);
				const uri = reference.uri ? matches.find(uri => uri.toString() === reference.uri) : undefined;
				if (reference.uri && !uri) { return; }
				const position = { source: reference.source, line: reference.line, column: reference.column };
				const trace = Number.isInteger(reference.generatedLine) && reference.generatedLine! >= 0 && reference.generatedLine! < current.document.lineCount &&
					Number.isInteger(reference.generatedColumn) && reference.generatedColumn! >= 0
					? service.trace(current, new vscode.Position(reference.generatedLine!, reference.generatedColumn!)) : undefined;
				await open({ mapped: current, position, uri, trace: trace && trace.original.source === position.source &&
					trace.original.line === position.line && trace.original.column === position.column ? trace : undefined });
			} catch (error) { void vscode.window.showErrorMessage(`Twofold: ${report(error)}`); }
		}),
		vscode.commands.registerCommand('twofold.configureSourceMap', async (reference?: SourceReference) => {
			try {
				const active = vscode.window.activeTextEditor;
				const current = reference ? await fromReference(reference) : (active && await service.load(active.document)) || mapped;
				if (!current) { void vscode.window.showInformationMessage('Open a generated file with a source map first.'); return; }
				const source = reference?.source ?? (await vscode.window.showQuickPick(current.map.sources.map((label, source) => ({ label, source })),
					{ title: 'Configure source mapping for' }))?.source;
				if (source !== undefined) { await configureSourceMapping(service, current, source); }
			} catch (error) { void vscode.window.showErrorMessage(`Twofold: ${report(error)}`); }
		}),
		vscode.commands.registerCommand('twofold.refreshSourceMap', () => { service.refresh(); }),
		vscode.commands.registerCommand('twofold.goToGeneratingSource', async () => {
			const editor = vscode.window.activeTextEditor;
			if (!editor) { return; }
			try {
				const current = await service.load(editor.document);
				const currentTrace = current && service.trace(current, editor.selection.active);
				if (current && currentTrace) { await open({ mapped: current, position: currentTrace.original, trace: currentTrace }); }
				else { void vscode.window.showInformationMessage('No generating source mapped at the cursor.'); }
			} catch (error) { void vscode.window.showErrorMessage(`Twofold: ${report(error)}`); }
		}),
		vscode.window.onDidChangeActiveTextEditor(editor => { clearHighlights(); void update(editor); }),
		vscode.window.onDidChangeTextEditorSelection(event => {
			if (event.textEditor.document.uri.toString() === mapped?.document.uri.toString()) { schedule(); }
		}),
		vscode.workspace.onDidChangeTextDocument(event => {
			if (event.document === vscode.window.activeTextEditor?.document) { schedule(); }
		}),
		service.onDidChange(schedule),
		vscode.languages.registerDefinitionProvider('*', {
			async provideDefinition(document, position, token) {
				try {
					const current = await service.load(document);
					const currentTrace = current && service.trace(current, position);
					if (!current || !currentTrace || token.isCancellationRequested) { return undefined; }
					const matches = await service.sourceUris(current, currentTrace.original.source);
					const targets = await Promise.all(matches.map(uri => service.target(current, currentTrace.original, currentTrace, uri)));
					if (token.isCancellationRequested) { return undefined; }
					return targets.map(target => {
						sourceDocuments.add(target.document.uri.toString());
						return { originSelectionRange: new vscode.Range(position.line, currentTrace.segment.column,
						position.line, Math.max(currentTrace.segment.column, currentTrace.endColumn)),
						targetUri: target.document.uri, targetRange: target.section.union(target.selection), targetSelectionRange: target.selection };
					});
				} catch (error) { output.appendLine(report(error)); return undefined; }
			}
		}),
		vscode.languages.registerHoverProvider('*', {
			async provideHover(document, position, token) {
				try {
					const current = await service.load(document);
					const currentTrace = current && service.trace(current, position);
					if (!current || !currentTrace || token.isCancellationRequested) { return undefined; }
					const markdown = new vscode.MarkdownString();
					markdown.isTrusted = { enabledCommands: ['twofold.configureSourceMap', 'twofold.openMappedSource'] };
					const appendActions = (source: SourcePosition, uri?: vscode.Uri) => {
						const reference: OpenReference = { generated: document.uri.toString(), source: source.source,
							line: source.line, column: source.column, uri: uri?.toString(), generatedLine: position.line, generatedColumn: position.character };
						markdown.appendMarkdown(` [Open](${commandLink('twofold.openMappedSource', reference)}) · ` +
							`[Configure source mapping…](${configurationLink(current, source.source)})\n\n`);
					};
					markdown.appendMarkdown('**Generating source**\n\n');
					markdown.appendText(locationLabel(current, currentTrace.original));
					appendActions(currentTrace.original);
					try {
						const matches = await service.sourceUris(current, currentTrace.original.source);
						if (matches.length > 1) { markdown.appendMarkdown(`**${matches.length} matching sources**\n\n`); }
						for (const [index, uri] of matches.entries()) {
							markdown.appendText(vscode.workspace.asRelativePath(uri, true));
							appendActions(currentTrace.original, uri);
							if (index >= 3) { continue; }
							const target = await service.target(current, currentTrace.original, currentTrace, uri);
							const start = Math.max(0, target.selection.start.line - 2);
							const end = Math.min(target.document.lineCount - 1, target.selection.end.line + 2);
							markdown.appendCodeblock(target.document.getText(new vscode.Range(start, 0, end,
								target.document.lineAt(end).text.length)).slice(0, 4000), target.document.languageId);
						}
					} catch (error) { markdown.appendMarkdown('\n\n'); markdown.appendText(report(error)); }
					if (currentTrace.stack.length) {
						markdown.appendMarkdown('\n\n**Generation stack** (innermost first)\n\n');
						for (const frame of currentTrace.stack.slice(0, 30)) {
							markdown.appendText(locationLabel(current, frame)); appendActions(frame);
						}
					}
					if (token.isCancellationRequested) { return undefined; }
					return new vscode.Hover(markdown, new vscode.Range(position.line, currentTrace.segment.column,
						position.line, Math.max(currentTrace.segment.column, currentTrace.endColumn)));
				} catch { return undefined; }
			}
		})
	);
	void update();
}
