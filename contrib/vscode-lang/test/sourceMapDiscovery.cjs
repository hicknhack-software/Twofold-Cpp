// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

const assert = require('node:assert/strict');
const vscode = require('vscode');
const { SourceMapService } = require('../out/sourceMapService');
const { saveSourceBinding, configureSourceMapping } = require('../out/sourceMapConfiguration');

exports.run = async workspace => {
	const service = new SourceMapService();
	const config = vscode.workspace.getConfiguration('twofold.sourceMap', workspace);
	const originalRoots = config.inspect('roots');
	const originalBindings = config.inspect('bindings');
	const write = async (uri, text) => {
		await vscode.workspace.fs.createDirectory(vscode.Uri.joinPath(uri, '..'));
		await vscode.workspace.fs.writeFile(uri, Buffer.from(text));
		return uri;
	};
	const makeMapped = async (file, source) => {
		const uri = vscode.Uri.joinPath(workspace, 'lookup', file);
		await write(uri, 'value\n//# sourceMappingURL=output.map\n');
		await write(vscode.Uri.joinPath(uri, '../output.map'), JSON.stringify({ version: 3, sources: [source], mappings: 'AAAA' }));
		return service.load(await vscode.workspace.openTextDocument(uri));
	};
	const position = new vscode.Position(0, 0);
	const waitForCount = async (mapped, count) => {
		const deadline = Date.now() + 5000;
		while ((await service.sourceUris(mapped, 0)).length !== count) {
			assert.ok(Date.now() < deadline, 'Workspace creation/deletion refreshes cached discovery');
			await new Promise(resolve => setTimeout(resolve, 25));
		}
	};
	try {
		await config.update('roots', { ...originalRoots.workspaceValue, 'sentinel://': 'retained' }, vscode.ConfigurationTarget.Workspace);
		const unique = await write(vscode.Uri.joinPath(workspace, 'lookup/a/nested/unique.protlr3'), 'value');
		const one = await makeMapped('one/output.cpp', 'test://unique.protlr3');
		assert.deepEqual((await service.sourceUris(one, 0)).map(uri => uri.toString()), [unique.toString()]);
		const definitions = await vscode.commands.executeCommand('vscode.executeDefinitionProvider', one.document.uri, position);
		assert.equal(definitions.length, 1);
		assert.equal(definitions[0].targetUri.toString(), unique.toString());
		const editor = await vscode.window.showTextDocument(one.document);
		editor.selection = new vscode.Selection(position, position);
		await vscode.commands.executeCommand('twofold.goToGeneratingSource');
		assert.equal(vscode.window.activeTextEditor.document.uri.toString(), unique.toString());

		const second = await write(vscode.Uri.joinPath(workspace, 'lookup/b/nested/unique.protlr3'), 'value');
		await waitForCount(one, 2);
		const expected = [unique, second].map(uri => uri.toString()).sort();
		assert.deepEqual((await service.sourceUris(one, 0)).map(uri => uri.toString()).sort(), expected);
		let multiple;
		const deadline = Date.now() + 5000;
		do {
			multiple = await vscode.commands.executeCommand('vscode.executeDefinitionProvider', one.document.uri, position);
			if (multiple.length === 2) { break; }
			await new Promise(resolve => setTimeout(resolve, 25));
		} while (Date.now() < deadline);
		assert.deepEqual(multiple.map(link => link.targetUri.toString()).sort(), expected);
		const hovers = await vscode.commands.executeCommand('vscode.executeHoverProvider', one.document.uri, position);
		const markdown = hovers.flatMap(item => item.contents).find(item => /matching sources/.test(item.value));
		assert.ok(markdown);
		assert.match(markdown.value, /2 matching sources/);
		assert.match(markdown.value, /lookup\/a\/nested\/unique.protlr3/);
		assert.match(markdown.value, /lookup\/b\/nested\/unique.protlr3/);
		assert.deepEqual(markdown.isTrusted.enabledCommands.sort(), ['twofold.configureSourceMap', 'twofold.openMappedSource']);
		const links = [...markdown.value.matchAll(/command:(twofold\.[\w]+)\?([^)]*)/g)].map(match => ({
			command: match[1], args: JSON.parse(decodeURIComponent(match[2]))
		}));
		const link = links.find(item => item.command === 'twofold.openMappedSource' && item.args[0].uri === second.toString());
		assert.ok(link, 'Each match has a working hover action');
		await vscode.commands.executeCommand(link.command, ...link.args);
		assert.equal(vscode.window.activeTextEditor.document.uri.toString(), second.toString());
		const configureLink = links.find(item => item.command === 'twofold.configureSourceMap');
		assert.deepEqual(configureLink.args, [{ generated: one.document.uri.toString(), source: 0 }]);

		const specific = await makeMapped('specific/output.cpp', 'test://a/nested/unique.protlr3');
		assert.equal((await service.sourceUri(specific, 0)).toString(), unique.toString(), 'Full source path narrows basename matches');
		const special = await write(vscode.Uri.joinPath(workspace, 'lookup/hidden/[literal].twofold'), 'value');
		const specialMap = await makeMapped('special/output.cpp', 'test://[literal].twofold');
		assert.equal((await service.sourceUri(specialMap, 0)).toString(), special.toString());
		const missing = await makeMapped('missing/output.cpp', 'test://not-present.protlr3');
		const missingHovers = await vscode.commands.executeCommand('vscode.executeHoverProvider', missing.document.uri, position);
		const missingText = missingHovers.flatMap(item => item.contents).map(item => item.value ?? item).join('\n');
		assert.match(missingText, /Cannot(?: |&nbsp;)find/);
		assert.match(missingText, /command:twofold.configureSourceMap/, 'Missing sources can be configured directly from hover');
		await vscode.workspace.fs.delete(second);
		await waitForCount(one, 1);

		const outside = vscode.Uri.joinPath(workspace, '../external');
		const roots = {};
		for (const key of ['global', 'directory', 'nested', 'file', 'ui']) {
			roots[key] = vscode.Uri.joinPath(outside, key);
			await write(vscode.Uri.joinPath(roots[key], 'external.protlr3'), 'value');
		}
		const generated = await makeMapped('out/output.cpp', 'test://external.protlr3');
		const sibling = await makeMapped('out/sibling/output.cpp', 'test://external.protlr3');
		const nested = await makeMapped('out/deep/output.cpp', 'test://external.protlr3');
		const other = await makeMapped('outside/output.cpp', 'test://external.protlr3');
		const resolved = async item => (await service.sourceUri(item, 0)).toString();
		const externalFile = key => vscode.Uri.joinPath(roots[key], 'external.protlr3').toString();
		await saveSourceBinding(generated.document.uri, 'test://', roots.global, 'global');
		assert.equal(await resolved(other), externalFile('global'));
		assert.equal(vscode.workspace.getConfiguration('twofold.sourceMap', workspace).inspect('roots').globalValue['sentinel://'], undefined,
			'Do not copy workspace settings into global settings');
		await saveSourceBinding(generated.document.uri, 'test://', roots.directory, 'directory');
		assert.equal(await resolved(generated), externalFile('directory'));
		assert.equal(await resolved(sibling), externalFile('directory'));
		assert.equal(await resolved(other), externalFile('global'), 'Directory boundary must not match outside/');
		await saveSourceBinding(nested.document.uri, 'test://', roots.nested, 'directory');
		assert.equal(await resolved(nested), externalFile('nested'));
		await saveSourceBinding(generated.document.uri, 'test://', roots.file, 'file');
		assert.equal(await resolved(generated), externalFile('file'));
		assert.equal(await resolved(sibling), externalFile('directory'));
		const exact = vscode.Uri.joinPath(roots.file, 'external.protlr3');
		await saveSourceBinding(generated.document.uri, 'test://external.protlr3', exact, 'file');
		assert.equal(await resolved(generated), exact.toString(), 'Exact source binding selects a file');
		const stored = vscode.workspace.getConfiguration('twofold.sourceMap', workspace).get('bindings')[generated.document.uri.toString()];
		assert.ok(stored.roots['test://'] && stored.roots['test://external.protlr3'], 'Saving retains existing bindings');
		await configureSourceMapping(service, other, 0, {
			chooseScope: async items => { assert.equal(items.length, 4); return { scope: 'file' }; },
			choosePrefix: async (source, initial) => { assert.equal(source, 'test://external.protlr3'); assert.equal(initial, 'test://'); return initial; },
			chooseLocation: async (source, suffix) => { assert.equal(suffix, 'external.protlr3'); return roots.ui; }
		});
		assert.equal(await resolved(other), externalFile('ui'), 'Tooltip configuration workflow persists and applies');
		await saveSourceBinding(generated.document.uri, 'test://', roots.ui, 'workspace');
		assert.equal(vscode.workspace.getConfiguration('twofold.sourceMap', workspace).get('roots')['sentinel://'], 'retained');
		assert.equal(await resolved(other), externalFile('ui'));
		assert.equal(await resolved(sibling), externalFile('directory'), 'Directory bindings override workspace roots');
		console.log('PASS: workspace discovery, all definition/hover matches, literal filenames, tooltip actions, global/directory/file bindings and precedence');
	} finally {
		await config.update('roots', originalRoots.globalValue, vscode.ConfigurationTarget.Global);
		await config.update('roots', originalRoots.workspaceValue, vscode.ConfigurationTarget.Workspace);
		await config.update('bindings', originalBindings.workspaceValue, vscode.ConfigurationTarget.Workspace);
		service.dispose();
	}
};
