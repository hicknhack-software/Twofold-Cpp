// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

import * as path from 'path';
import { createHash } from 'crypto';
import * as vscode from 'vscode';
import { DecodedSourceMap, inlineSourceMap, sourceMapReference, SourcePosition, Trace } from './sourceMap';

export interface MappedDocument {
	document: vscode.TextDocument;
	mapUri: vscode.Uri;
	map: DecodedSourceMap;
}

export interface SourceTarget {
	document: vscode.TextDocument;
	selection: vscode.Range;
	section: vscode.Range;
}

export interface SourceBinding {
	scope: 'file' | 'directory';
	roots: Record<string, string>;
}

export function globLiteral(value: string): string { return value.replace(/[?*\[\]{}]/g, '[$&]'); }

export function resourceKey(uri: vscode.Uri): string {
	return uri.with({ path: uri.scheme === 'file' && process.platform === 'win32' ? uri.path.toLowerCase() : uri.path,
		query: '', fragment: '' }).toString();
}

function hasScheme(value: string): boolean { return /^[a-z][\w+.-]*:/i.test(value); }

/** Resolve URL references without treating Windows drive letters as URI schemes. */
export function resolveReference(base: vscode.Uri, reference: string): vscode.Uri {
	const value = reference.replace(/\\/g, '/');
	if (/^[a-z]:\//i.test(value) || value.startsWith('//')) { return vscode.Uri.file(value); }
	if (hasScheme(value)) { return vscode.Uri.parse(value, true); }
	const url = new URL(value, base.toString());
	return vscode.Uri.parse(url.toString(), true);
}

export class SourceMapService implements vscode.Disposable {
	private readonly cache = new Map<string, { version: number; value: Promise<MappedDocument | undefined> }>();
	private readonly mapWatchers = new Map<string, vscode.FileSystemWatcher>();
	private readonly embedded = new Map<string, string>();
	private readonly searches = new Map<string, Promise<vscode.Uri[]>>();
	private readonly changed = new vscode.EventEmitter<void>();
	readonly onDidChange = this.changed.event;
	private readonly disposables: vscode.Disposable[];

	constructor() {
		const changedMap = (uri: vscode.Uri) => {
			if (this.mapWatchers.has(uri.toString())) { this.refresh(); }
		};
		this.disposables = [
			vscode.workspace.onDidChangeWorkspaceFolders(() => this.refresh()),
			vscode.workspace.onDidChangeTextDocument(event => {
				this.cache.delete(event.document.uri.toString()); changedMap(event.document.uri);
			}),
			vscode.workspace.onDidCloseTextDocument(document => {
				this.cache.delete(document.uri.toString()); changedMap(document.uri);
			}),
			vscode.workspace.onDidChangeConfiguration(event => {
				if (event.affectsConfiguration('twofold.sourceMap')) { this.refresh(); }
			}),
			vscode.workspace.registerTextDocumentContentProvider('twofold-source', {
				provideTextDocumentContent: uri => this.embedded.get(uri.toString()) ?? ''
			}), this.changed
		];
		const watcher = vscode.workspace.createFileSystemWatcher('**/*');
		this.disposables.push(watcher, watcher.onDidCreate(() => this.refresh()), watcher.onDidDelete(() => this.refresh()));
	}

	refresh(): void { this.cache.clear(); this.searches.clear(); this.changed.fire(); }

	async load(document: vscode.TextDocument): Promise<MappedDocument | undefined> {
		if (document.uri.scheme === 'twofold-source') { return undefined; }
		const key = document.uri.toString();
		const cached = this.cache.get(key);
		if (cached?.version === document.version) { return cached.value; }
		const value = this.read(document);
		// Keep only a small working set, including negative lookups for ordinary files.
		if (this.cache.size >= 32) { this.cache.delete(this.cache.keys().next().value!); }
		this.cache.set(key, { version: document.version, value });
		return value;
	}

	private async read(document: vscode.TextDocument): Promise<MappedDocument | undefined> {
		const reference = sourceMapReference(document.getText());
		if (!reference) { return undefined; }
		const inline = inlineSourceMap(reference);
		const mapUri = inline === undefined ? resolveReference(document.uri, reference) : document.uri;
		if (inline === undefined && !this.mapWatchers.has(mapUri.toString())) {
			const watcher = vscode.workspace.createFileSystemWatcher(new vscode.RelativePattern(
				vscode.Uri.joinPath(mapUri, '..'), path.posix.basename(mapUri.path).replace(/[?*\[\]{}]/g, '[$&]')));
			this.mapWatchers.set(mapUri.toString(), watcher);
			for (const event of [watcher.onDidChange, watcher.onDidCreate, watcher.onDidDelete]) {
				this.disposables.push(event(() => this.refresh()));
			}
		}
		const open = vscode.workspace.textDocuments.find(item => item.uri.toString() === mapUri.toString());
		const text = inline ?? (open ? open.getText() : Buffer.from(await vscode.workspace.fs.readFile(mapUri)).toString('utf8'));
		return { document, mapUri, map: new DecodedSourceMap(text) };
	}

	trace(mapped: MappedDocument, position: vscode.Position): Trace | undefined {
		return mapped.map.traceAt(position.line, position.character, mapped.document.lineAt(position.line).text.length);
	}

	private async isFile(uri: vscode.Uri): Promise<boolean> {
		try { return !!((await vscode.workspace.fs.stat(uri)).type & vscode.FileType.File); } catch { return false; }
	}

	sourceName(mapped: MappedDocument, index: number): string {
		const name = mapped.map.sources[index].replace(/\\/g, '/');
		const root = mapped.map.sourceRoot.replace(/\\/g, '/');
		return !root || hasScheme(name) || name.startsWith('/') ? name
			: `${root}${root.endsWith('/') ? '' : '/'}${name}`;
	}

	private configuredRoot(mapped: MappedDocument, source: string): { prefix: string; root: string } | undefined {
		const resource = vscode.workspace.getWorkspaceFolder(mapped.document.uri)?.uri
			?? vscode.workspace.workspaceFolders?.[0]?.uri ?? mapped.document.uri;
		const config = vscode.workspace.getConfiguration('twofold.sourceMap', resource);
		const bindings = config.get<Record<string, SourceBinding>>('bindings', {});
		const generated = mapped.document.uri;
		const pathKey = (uri: vscode.Uri) => uri.scheme === 'file' && process.platform === 'win32' ? uri.path.toLowerCase() : uri.path;
		const applicable = Object.entries(bindings).flatMap(([key, binding]) => {
			if (!binding || !['file', 'directory'].includes(binding.scope)) { return []; }
			const uri = resolveReference(mapped.mapUri, key);
			if (uri.scheme !== generated.scheme || uri.authority !== generated.authority) { return []; }
			const base = pathKey(uri).replace(/\/$/, '');
			if (binding.scope === 'file' ? pathKey(generated) !== pathKey(uri) : !pathKey(generated).startsWith(base + '/')) { return []; }
			return [{ binding, length: base.length }];
		}).sort((a, b) => Number(b.binding.scope === 'file') - Number(a.binding.scope === 'file') || b.length - a.length);
		for (const roots of [...applicable.map(item => item.binding.roots), config.get<Record<string, string>>('roots', {})]) {
			const prefix = Object.keys(roots ?? {}).filter(key => source.startsWith(key) && typeof roots[key] === 'string')
				.sort((a, b) => b.length - a.length)[0];
			if (prefix !== undefined) { return { prefix, root: roots[prefix] }; }
		}
		return undefined;
	}

	private async workspaceSources(source: string): Promise<vscode.Uri[]> {
		const relative = decodeURIComponent(source.replace(/^[a-z][\w+.-]*:\/\//i, '')).replace(/\\/g, '/').replace(/^\/+/, '');
		const basename = path.posix.basename(relative);
		if (!basename || basename === '.' || basename === '..') { return []; }
		let search = this.searches.get(basename);
		if (!search) {
			// Search every workspace folder, including normally excluded generated/test folders.
			search = Promise.resolve(vscode.workspace.findFiles(`**/${globLiteral(basename)}`, null))
				.then(uris => [...new Map(uris.map(uri => [uri.toString(), uri])).values()]
					.sort((a, b) => a.toString().localeCompare(b.toString())));
			if (this.searches.size >= 64) { this.searches.delete(this.searches.keys().next().value!); }
			this.searches.set(basename, search);
		}
		const matches = await search;
		const suffixMatches = matches.filter(uri => uri.path.endsWith('/' + relative));
		return suffixMatches.length ? suffixMatches : matches;
	}

	async sourceUris(mapped: MappedDocument, index: number): Promise<vscode.Uri[]> {
		const source = this.sourceName(mapped, index);
		const configured = this.configuredRoot(mapped, source);
		const folder = vscode.workspace.getWorkspaceFolder(mapped.document.uri) ?? vscode.workspace.workspaceFolders?.[0];
		if (configured) {
			const root = configured.root.replace(/\\/g, '/');
			const rootUri = hasScheme(root) || path.isAbsolute(root)
				? resolveReference(mapped.mapUri, root) : vscode.Uri.joinPath(folder?.uri ?? vscode.Uri.joinPath(mapped.mapUri, '..'), root);
			const uri = vscode.Uri.joinPath(rootUri, decodeURIComponent(source.slice(configured.prefix.length)).replace(/^\/+/, ''));
			if (await this.isFile(uri)) { return [uri]; }
			throw new Error(`Configured source ${source} does not exist at ${uri.fsPath}. Configure its source mapping again.`);
		} else {
			const resolved = resolveReference(mapped.mapUri, source);
			// Standard relative/absolute references remain authoritative when they exist.
			if (['file', 'vscode-remote'].includes(resolved.scheme) && await this.isFile(resolved)) { return [resolved]; }
			const matches = await this.workspaceSources(source);
			if (matches.length) { return matches; }
		}
		// Embedded snapshots also work when templates live on another build machine.
		const content = mapped.map.sourcesContent[index];
		if (typeof content === 'string') {
			const identity = createHash('sha256').update(`${mapped.mapUri.toString()}\n${index}\n${content}`).digest('hex');
			const uri = vscode.Uri.from({ scheme: 'twofold-source', path: '/' + path.posix.basename(source),
				query: identity });
			this.embedded.set(uri.toString(), content);
			return [uri];
		}
		throw new Error(`Cannot find ${source} in the workspace. Use Configure Source Mapping or twofold.sourceMap.roots for external sources.`);
	}

	async sourceUri(mapped: MappedDocument, index: number): Promise<vscode.Uri> {
		const uris = await this.sourceUris(mapped, index);
		if (uris.length !== 1) { throw new Error(`Multiple sources match ${this.sourceName(mapped, index)}. Choose a source from the matches.`); }
		return uris[0];
	}

	async target(mapped: MappedDocument, position: SourcePosition, trace?: Trace, uri?: vscode.Uri): Promise<SourceTarget> {
		const document = await vscode.workspace.openTextDocument(uri ?? await this.sourceUri(mapped, position.source));
		const start = document.validatePosition(new vscode.Position(position.line, position.column));
		const word = document.getWordRangeAtPosition(start);
		const selection = word ?? document.validateRange(new vscode.Range(start, start.translate(0, 1)));
		let section = selection;
		if (trace?.segment.interpolate && trace.segment.original) {
			const origin = trace.segment.original;
			section = document.validateRange(new vscode.Range(origin.line, origin.column, origin.line,
				origin.column + Math.max(1, trace.endColumn - trace.segment.column)));
		} else {
			const line = document.lineAt(start.line).text;
			if (line.slice(start.character).startsWith('#{')) {
				const end = line.indexOf('}', start.character + 2);
				section = new vscode.Range(start, new vscode.Position(start.line, end < 0 ? line.length : end + 1));
			}
		}
		return { document, selection, section };
	}

	dispose(): void {
		this.disposables.forEach(item => item.dispose());
		this.mapWatchers.forEach(watcher => watcher.dispose());
		this.cache.clear(); this.embedded.clear(); this.searches.clear();
	}
}
