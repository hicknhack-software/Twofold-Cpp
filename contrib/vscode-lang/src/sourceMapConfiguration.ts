// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

import * as path from 'path';
import * as vscode from 'vscode';
import { MappedDocument, resourceKey, resolveReference, SourceBinding, SourceMapService } from './sourceMapService';

export type BindingScope = 'file' | 'directory' | 'workspace' | 'global';
export interface SourceReference { generated: string; source: number; }

export function commandLink(command: string, argument: unknown): string {
	return `command:${command}?${encodeURIComponent(JSON.stringify([argument]))}`;
}

export function configurationLink(mapped: MappedDocument, source: number): string {
	return commandLink('twofold.configureSourceMap', { generated: mapped.document.uri.toString(), source });
}

export async function saveSourceBinding(generated: vscode.Uri, source: string, root: vscode.Uri, scope: BindingScope): Promise<void> {
	const config = vscode.workspace.getConfiguration('twofold.sourceMap', generated);
	const folder = vscode.workspace.getWorkspaceFolder(generated) ?? vscode.workspace.workspaceFolders?.[0];
	const target = scope === 'global' || !folder ? vscode.ConfigurationTarget.Global
		: vscode.workspace.workspaceFile ? vscode.ConfigurationTarget.WorkspaceFolder : vscode.ConfigurationTarget.Workspace;
	const configuration = target !== vscode.ConfigurationTarget.Global
		? vscode.workspace.getConfiguration('twofold.sourceMap', folder!.uri) : config;
	const own = <T>(key: string): T | undefined => {
		const values = configuration.inspect<T>(key);
		return target === vscode.ConfigurationTarget.Global ? values?.globalValue
			: target === vscode.ConfigurationTarget.Workspace ? values?.workspaceValue : values?.workspaceFolderValue;
	};
	if (scope === 'global' || scope === 'workspace') {
		await configuration.update('roots', { ...own<Record<string, string>>('roots'), [source]: root.toString() }, target);
	} else {
		const bindings = own<Record<string, SourceBinding>>('bindings') ?? {};
		const uri = scope === 'file' ? generated : vscode.Uri.joinPath(generated, '..');
		const key = Object.keys(bindings).find(key => bindings[key].scope === scope &&
			resourceKey(resolveReference(generated, key)) === resourceKey(uri)) ?? uri.toString();
		await configuration.update('bindings', { ...bindings,
			[key]: { scope, roots: { ...bindings[key]?.roots, [source]: root.toString() } }
		}, target);
	}
}

// Keep prompts injectable so extension-host checks can verify the complete workflow.
export interface ConfigurationPrompts {
	chooseScope(items: { label: string; description: string; scope: BindingScope }[]): Thenable<{ scope: BindingScope } | undefined>;
	choosePrefix(source: string, initial: string): Thenable<string | undefined>;
	chooseLocation(source: string, suffix: string): Thenable<vscode.Uri | undefined>;
}

const prompts: ConfigurationPrompts = {
	chooseScope: items => vscode.window.showQuickPick(items, { title: 'Apply source mapping to', matchOnDescription: true }),
	choosePrefix: (source, initial) => vscode.window.showInputBox({ title: 'Source prefix to map', value: initial,
		prompt: `Source: ${source}. Enter its full name to bind one source file.`,
		validateInput: value => source.startsWith(value) ? undefined : 'The prefix must match the beginning of this source name.' }),
	chooseLocation: async (source, suffix) => (await vscode.window.showOpenDialog({
		title: suffix ? `Select the source root containing ${suffix}` : `Select the file for ${source}`,
		canSelectFolders: !!suffix, canSelectFiles: !suffix, canSelectMany: false,
		openLabel: suffix ? 'Use Source Root' : 'Use Source File'
	}))?.[0]
};

export async function configureSourceMapping(service: SourceMapService, mapped: MappedDocument, index: number,
	ui: ConfigurationPrompts = prompts): Promise<void> {
	const source = service.sourceName(mapped, index);
	const generated = mapped.document.uri;
	const scope = await ui.chooseScope([
		{ label: 'This generated file', description: path.posix.basename(generated.path), scope: 'file' },
		{ label: 'This generated directory', description: vscode.Uri.joinPath(generated, '..').fsPath, scope: 'directory' },
		{ label: 'This workspace', description: 'All generated files in this workspace folder', scope: 'workspace' },
		{ label: 'Globally', description: 'All workspaces', scope: 'global' }
	]);
	if (!scope) { return; }
	const initial = /^[a-z][\w+.-]*:\/\//i.exec(source)?.[0] ?? source.slice(0, source.lastIndexOf('/') + 1);
	const prefix = await ui.choosePrefix(source, initial);
	if (prefix === undefined || !source.startsWith(prefix)) { return; }
	const root = await ui.chooseLocation(source, source.slice(prefix.length));
	if (!root) { return; }
	await saveSourceBinding(generated, prefix, root, scope.scope);
	service.refresh();
}
