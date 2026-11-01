// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { spawn } = require('node:child_process');

const executable = process.env.VSCODE_EXECUTABLE;
if (!executable || !fs.existsSync(executable)) {
	console.error('Set VSCODE_EXECUTABLE to an installed VS Code executable to run extension host tests.');
	process.exit(1);
}
const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'twofold-vscode-test-'));
const workspace = path.join(directory, 'workspace');
const roots = {};
const bindings = {};
if (process.env.TWOFOLD_REAL_FIXTURES) {
	const { verifyFixtures } = require('./realFixtures.cjs');
	const fixtures = verifyFixtures(process.env.TWOFOLD_REAL_FIXTURES);
	fs.cpSync(process.env.TWOFOLD_REAL_FIXTURES, path.join(workspace, 'real'), { recursive: true });
	// Resolve templates by workspace discovery. Limit the input root to this output directory.
	bindings[vscodeFileUri(path.join(workspace, 'real'))] = { scope: 'directory', roots: { 'protlr3://': 'real/sources/protlr3' } };
}
function vscodeFileUri(file) {
	const { pathToFileURL } = require('node:url');
	return pathToFileURL(file).toString();
}
fs.mkdirSync(path.join(workspace, '.vscode'), { recursive: true });
fs.mkdirSync(path.join(workspace, 'templates'));
fs.mkdirSync(path.join(workspace, 'inputs'));
fs.writeFileSync(path.join(workspace, '.vscode', 'settings.json'), JSON.stringify({
	'twofold.sourceMap.roots': roots,
	'twofold.sourceMap.bindings': bindings,
	'workbench.startupEditor': 'none', 'security.workspace.trust.enabled': false
}));
fs.writeFileSync(path.join(workspace, 'templates', 'leaf.hpp.twofold'), 'prefix é😀 hello #{value}\ncaller\n');
fs.writeFileSync(path.join(workspace, 'templates', 'root.hpp.twofold'), 'root\ncaller\n');
fs.writeFileSync(path.join(workspace, 'inputs', 'protocol.txt'), 'protocol');
fs.writeFileSync(path.join(workspace, 'generated.cpp'), 'é😀 hello\nunmapped\n//# sourceMappingURL=generated%20file.map\n');
fs.writeFileSync(path.join(workspace, 'generated file.map'), JSON.stringify({
	version: 3,
	sources: ['template://leaf.hpp.twofold', 'template://root.hpp.twofold', 'input://protocol.txt'],
	mappings: 'AAAO', x_de_hicknhack_software_column_interpolation: 'C',
	x_de_hicknhack_software_callstack: { callers: 'CCAC;CDA', indices: 'A' }
}));
const environment = { ...process.env };
delete environment.ELECTRON_RUN_AS_NODE;
const child = spawn(executable, [
	workspace, '--new-window', '--disable-gpu', '--disable-extensions', '--skip-welcome', '--skip-release-notes',
	'--disable-workspace-trust', '--no-sandbox',
	`--user-data-dir=${path.join(directory, 'user')}`, `--extensions-dir=${path.join(directory, 'extensions')}`,
	`--extensionDevelopmentPath=${path.resolve(__dirname, '..')}`,
	`--extensionTestsPath=${path.join(__dirname, 'extensionHost.cjs')}`
], { env: environment, stdio: ['ignore', 'pipe', 'pipe'], windowsHide: true });
const logs = [];
const timeout = setTimeout(() => {
	console.error('VS Code extension host tests timed out after 120 seconds.');
	child.kill();
}, 120000);
for (const stream of [child.stdout, child.stderr]) {
	stream.on('data', chunk => {
		logs.push(chunk.toString());
		for (const line of chunk.toString().split('\n')) {
			if (/PASS:|AssertionError|Error:|\bat exports\.run|mandatory|does not exist/.test(line)) { console.log(line); }
		}
	});
}
child.on('error', error => { clearTimeout(timeout); console.error(error); process.exitCode = 1; });
child.on('exit', code => {
	clearTimeout(timeout);
	process.exitCode = code ?? 1;
	// The test folder is isolated and contains no user data. Leave it on failure for diagnosis.
	if (code === 0) { fs.rmSync(directory, { recursive: true, force: true, maxRetries: 5, retryDelay: 200 }); }
	else {
		fs.writeFileSync(path.join(directory, 'test.log'), logs.join(''));
		console.error(`Extension host test files and log: ${directory}`);
	}
});
