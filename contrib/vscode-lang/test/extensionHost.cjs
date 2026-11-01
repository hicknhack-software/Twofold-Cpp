// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

const assert = require('node:assert/strict');
const vscode = require('vscode');
const { resolveReference, SourceMapService } = require('../out/sourceMapService');

exports.run = async () => {
	const workspace = vscode.workspace.workspaceFolders[0].uri;
	const generated = await vscode.workspace.openTextDocument(vscode.Uri.joinPath(workspace, 'generated.cpp'));
	const extension = vscode.extensions.getExtension('hicknhack-software.twofold');
	assert.ok(extension);
	await extension.activate();
	const commands = await vscode.commands.getCommands();
	assert.ok(commands.includes('twofold.goToGeneratingSource'));
	assert.ok(commands.includes('twofold.sources.focus'));
	assert.ok(commands.includes('twofold.configureSourceMap'));

	const location = new vscode.Position(0, 4); // UTF-16 column following é + a surrogate pair + a space
	const definitions = await vscode.commands.executeCommand('vscode.executeDefinitionProvider', generated.uri, location);
	assert.equal(definitions.length, 1);
	const definition = definitions[0];
	assert.equal(definition.targetUri.toString(), vscode.Uri.joinPath(workspace, 'templates/leaf.hpp.twofold').toString());
	assert.equal(definition.targetSelectionRange.start.character, 11);
	assert.ok(definition.targetRange.contains(definition.targetSelectionRange));
	assert.equal((await vscode.commands.executeCommand('vscode.executeDefinitionProvider', generated.uri, new vscode.Position(1, 0))).length, 0);
	const hovers = await vscode.commands.executeCommand('vscode.executeHoverProvider', generated.uri, location);
	const hover = hovers.flatMap(item => item.contents).map(item => item.value ?? item).join('\n');
	assert.match(hover, /Generation stack/);
	assert.match(hover, /template:\/\/root.hpp.twofold:2:1/);
	assert.match(hover, /input:\/\/protocol.txt:1:1/);
	assert.match(hover, /prefix é😀 hello/);

	const editor = await vscode.window.showTextDocument(generated);
	editor.selection = new vscode.Selection(location, location);
	await vscode.commands.executeCommand('twofold.goToGeneratingSource');
	assert.equal(vscode.window.activeTextEditor.document.uri.toString(), definition.targetUri.toString());
	assert.equal(vscode.window.activeTextEditor.selection.start.character, 11);
	await vscode.commands.executeCommand('twofold.sources.focus');
	console.log('PASS: panel commands, UTF-16 definition/peek locations, hover previews, caller stacks, source navigation');

	assert.equal(resolveReference(generated.uri, 'generated%20file.map').toString(), vscode.Uri.joinPath(workspace, 'generated file.map').toString());
	assert.equal(resolveReference(generated.uri, 'C:\\build\\out.map').scheme, 'file');
	assert.equal(resolveReference(vscode.Uri.parse('vscode-remote://ssh-remote+host/build/out.cpp'), '../out.map').toString(),
		vscode.Uri.parse('vscode-remote://ssh-remote+host/out.map').toString());

	// Exercise filesystem loading, configured roots, missing sources, embedded snapshots,
	// unsaved map changes, and recovery from a map that was initially absent.
	const service = new SourceMapService();
	try {
		const mapped = await service.load(generated);
		assert.equal(mapped.map.sources.length, 3);
		const rooted = { ...mapped, map: { ...mapped.map, sources: ['leaf.hpp.twofold'], sourceRoot: 'template://' } };
		assert.equal((await service.sourceUri(rooted, 0)).toString(), definition.targetUri.toString());
		assert.equal((await service.target(mapped, { source: 2, line: 0, column: 0 })).document.getText(), 'protocol');
		await vscode.commands.executeCommand('twofold.openSource', { mapped, position: { source: 2, line: 0, column: 0 } });
		assert.equal(vscode.window.activeTextEditor.document.getText(), 'protocol');
		const sourceTarget = await service.target(mapped, service.trace(mapped, location).original, service.trace(mapped, location));
		assert.equal(sourceTarget.document.getText(sourceTarget.section), 'é😀 hello');
		const mapDocument = await vscode.workspace.openTextDocument(mapped.mapUri);
		const replacement = { version: 3, sources: ['missing.twofold'], mappings: 'AAAA', sourcesContent: ['snapshot #{value}'] };
		const edit = new vscode.WorkspaceEdit();
		edit.replace(mapDocument.uri, new vscode.Range(0, 0, mapDocument.lineCount, 0), JSON.stringify(replacement));
		assert.ok(await vscode.workspace.applyEdit(edit));
		const changed = await service.load(generated);
		assert.equal(changed.map.sources[0], 'missing.twofold');
		const snapshot = await service.target(changed, { source: 0, line: 0, column: 0 });
		assert.equal(snapshot.document.uri.scheme, 'twofold-source');
		assert.equal(snapshot.document.getText(), 'snapshot #{value}');
		const noSnapshot = { ...changed, map: { ...changed.map, sourcesContent: [] } };
		await assert.rejects(service.sourceUri(noSnapshot, 0), /twofold.sourceMap.roots/);
		const relativeUri = vscode.Uri.joinPath(workspace, 'relative.cpp');
		await vscode.workspace.fs.writeFile(relativeUri, Buffer.from('text\n//# sourceMappingURL=late.map\n'));
		const relative = await vscode.workspace.openTextDocument(relativeUri);
		await assert.rejects(service.load(relative));
		const refreshed = new Promise((resolve, reject) => {
			const timeout = setTimeout(() => { subscription.dispose(); reject(new Error('Map creation watcher did not refresh')); }, 5000);
			const subscription = service.onDidChange(() => { clearTimeout(timeout); subscription.dispose(); resolve(); });
		});
		await vscode.workspace.fs.writeFile(vscode.Uri.joinPath(workspace, 'late.map'), Buffer.from(JSON.stringify({
			version: 3, sourceRoot: 'templates', sources: ['leaf.hpp.twofold'], mappings: 'AAAA'
		})));
		await refreshed;
		const recovered = await service.load(relative);
		assert.equal((await service.sourceUri(recovered, 0)).toString(), definition.targetUri.toString());
		const inline = await vscode.workspace.openTextDocument({ content: 'inline\n//# sourceMappingURL=data:application/json;base64,' +
			Buffer.from(JSON.stringify(replacement)).toString('base64') });
		assert.equal((await service.load(inline)).map.sources[0], 'missing.twofold');
		console.log('PASS: URI resolution, sourceRoot, root bindings, exact source highlights, embedded sources, live edits, map recovery, inline maps');
		await require('./sourceMapDiscovery.cjs').run(workspace);
		if (process.env.TWOFOLD_REAL_FIXTURES) {
			const { readFixtures } = require('./realFixtures.cjs');
			const directory = vscode.Uri.joinPath(workspace, 'real');
			const fixtures = readFixtures(directory.fsPath);
			for (const item of fixtures.cases) {
				const document = await vscode.workspace.openTextDocument(vscode.Uri.joinPath(directory, item.file));
				const position = new vscode.Position(item.line, item.column);
				const current = await service.load(document);
				assert.ok(current, `${item.file}: sourceMappingURL loaded`);
				const trace = service.trace(current, position);
				const target = await service.target(current, trace.original, trace);
				const expected = vscode.Uri.joinPath(directory, fixtures.sources[item.source]);
				assert.equal(target.document.uri.toString(), expected.toString());
				assert.ok(target.section.contains(target.selection), `${item.file}: section ${JSON.stringify(target.section)} selection ${JSON.stringify(target.selection)}`);
				const links = await vscode.commands.executeCommand('vscode.executeDefinitionProvider', document.uri, position);
				const link = links.find(link => link.targetUri?.toString() === expected.toString());
				assert.ok(link, `${item.file}: template definition alongside any native language definitions`);
				assert.ok(link.targetSelectionRange.isEqual(target.selection));
				const hovers = await vscode.commands.executeCommand('vscode.executeHoverProvider', document.uri, position);
				assert.match(hovers.flatMap(hover => hover.contents).map(content => content.value ?? content).join('\n'), /Generating source/);
				const editor = await vscode.window.showTextDocument(document);
				editor.selection = new vscode.Selection(position, position);
				await vscode.commands.executeCommand('twofold.goToGeneratingSource');
				assert.equal(vscode.window.activeTextEditor.document.uri.toString(), expected.toString());
				assert.ok(vscode.window.activeTextEditor.selection.isEqual(new vscode.Selection(target.selection.start, target.selection.end)));
				if (item.input) {
					const frame = trace.stack.find(frame => current.map.sources[frame.source] === item.input.source &&
						frame.line === item.input.line && frame.column === item.input.column);
					assert.ok(frame, `${item.file}: input stack frame`);
					await vscode.commands.executeCommand('twofold.openSource', { mapped: current, position: frame });
					assert.equal(vscode.window.activeTextEditor.document.uri.toString(),
						vscode.Uri.joinPath(directory, fixtures.sources[item.input.source]).toString());
					assert.equal(vscode.window.activeTextEditor.selection.start.line, item.input.line);
					assert.equal(vscode.window.activeTextEditor.selection.start.character, item.input.column);
				}
				for (let index = 0; index < current.map.sources.length; index++) {
					assert.equal((await service.sourceUri(current, index)).toString(),
						vscode.Uri.joinPath(directory, fixtures.sources[current.map.sources[index]]).toString());
				}
				console.log(`PASS: real ${item.file} definitions, hover, generating-source and input-frame clicks`);
			}
		}
	} finally { service.dispose(); }
};
