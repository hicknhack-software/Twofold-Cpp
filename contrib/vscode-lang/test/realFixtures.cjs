// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { DecodedSourceMap, sourceMapReference } = require('../out/sourceMap');

exports.readFixtures = directory => {
	const manifest = JSON.parse(fs.readFileSync(path.join(directory, 'manifest.json'), 'utf8'));
	const cases = manifest.cases.map(item => {
		const text = fs.readFileSync(path.join(directory, item.file), 'utf8');
		const index = text.indexOf(item.needle);
		assert.ok(index >= 0, `${item.file}: missing ${item.needle}`);
		const before = text.slice(0, index + (item.offset ?? 0));
		return { ...item, line: before.split('\n').length - 1, column: before.length - before.lastIndexOf('\n') - 1,
			expectedLine: item.line, expectedColumn: item.column };
	});
	return { ...manifest, cases };
};

exports.verifyFixtures = directory => {
	const fixtures = exports.readFixtures(directory);
	for (const file of fixtures.artifacts) {
		const text = fs.readFileSync(path.join(directory, file), 'utf8');
		assert.equal(sourceMapReference(text), path.basename(file) + '.map', file);
		const decoded = new DecodedSourceMap(fs.readFileSync(path.join(directory, file + '.map'), 'utf8'));
		assert.ok(decoded.lines.some(line => line.length), file);
		for (const source of decoded.sources) { assert.ok(fixtures.sources[source], source); }
	}
	for (const item of fixtures.cases) {
		const text = fs.readFileSync(path.join(directory, item.file), 'utf8');
		const decoded = new DecodedSourceMap(fs.readFileSync(path.join(directory, item.file + '.map'), 'utf8'));
		const trace = decoded.traceAt(item.line, item.column, text.split('\n')[item.line].length);
		assert.ok(trace, item.file);
		assert.equal(decoded.sources[trace.original.source], item.source, item.file);
		if (item.input) {
			assert.ok(trace.stack.some(frame => decoded.sources[frame.source] === item.input.source &&
				frame.line === item.input.line && frame.column === item.input.column), `${item.file}: exact input frame`);
		} else {
			assert.equal(trace.original.line, item.expectedLine);
			assert.equal(trace.original.column, item.expectedColumn);
		}
	}
	return fixtures;
};
