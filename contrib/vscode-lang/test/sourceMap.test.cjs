// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

const { test } = require('node:test');
const assert = require('node:assert/strict');
const { DecodedSourceMap, decodeVlq, inlineSourceMap, sourceMapReference } = require('../out/sourceMap');

function map(overrides = {}) {
	return new DecodedSourceMap(JSON.stringify({ version: 3, sources: ['template://leaf.twofold'],
		names: [], mappings: 'AAAA,KAAK,K;;AACA', ...overrides }));
}

test('reads the last standalone C++ or CSS sourceMappingURL comment, including CRLF', () => {
	assert.equal(sourceMapReference('//# sourceMappingURL=first.map\r\nint i;\r\n/*# sourceMappingURL=out%20file.map */\r\n'), 'out%20file.map');
	assert.equal(sourceMapReference('// comment\n//@ sourceMappingURL=x.map'), 'x.map');
	assert.equal(sourceMapReference('const s = "//# sourceMappingURL=wrong.map";'), undefined);
	assert.equal(sourceMapReference('ordinary file'), undefined);
	assert.equal(sourceMapReference('<html></html>\n<!--# sourceMappingURL=out.html.map -->\r\n'), 'out.html.map');
	assert.equal(sourceMapReference('<!--# sourceMappingURL=out.html.map-->'), 'out.html.map');
	assert.equal(sourceMapReference('<!--# sourceMappingURL=unfinished.map'), undefined);
});

test('decodes base64 and URL encoded inline maps', () => {
	const text = '{"version":3,"sources":["é😀"],"mappings":""}';
	assert.equal(inlineSourceMap('data:application/json;charset=utf-8;base64,' + Buffer.from(text).toString('base64')), text);
	assert.equal(inlineSourceMap('data:application/json,' + encodeURIComponent(text)), text);
	assert.equal(inlineSourceMap('test.map'), undefined);
	assert.throws(() => inlineSourceMap('data:text/html,hello'), /Unsupported/);
});

test('uses zero based UTF-16 columns, exact boundaries, and unmapped gaps', () => {
	const decoded = map({ x_de_hicknhack_software_column_interpolation: 'C,A,A;;C' });
	assert.equal(decoded.traceAt(0, 4, 20).original.column, 4);
	assert.equal(decoded.traceAt(0, 5, 20).original.column, 5);
	assert.equal(decoded.traceAt(0, 9, 20).original.column, 5); // no interpolation
	assert.equal(decoded.traceAt(0, 10, 20), undefined);
	assert.equal(decoded.traceAt(1, 0, 20), undefined); // never borrow a previous line's mapping
	assert.deepEqual(decoded.traceAt(2, 2, 8).original, { source: 0, line: 1, column: 7 });
	assert.equal(decoded.traceAt(3, 0, 20), undefined);
	assert.equal(map({ mappings: 'IAAA' }).traceAt(0, 3, 20), undefined);
	const unicode = map({ mappings: 'AAAA', x_de_hicknhack_software_column_interpolation: 'C' });
	assert.equal(unicode.traceAt(0, 'é😀'.length, 7).original.column, 3);
});

test('decodes SourceMap-Cpp nested callers, sibling roots, and empty extension slots', () => {
	const decoded = map({
		sources: ['template://leaf.twofold', 'template://root.twofold', 'input://protocol'],
		x_de_hicknhack_software_column_interpolation: 'C,A,A;;C',
		x_de_hicknhack_software_callstack: {
			callers: 'EAA;DSIA;AUEA;DhBJC',
			indices: 'G,,F;;C'
		}
	});
	assert.deepEqual(decoded.traceAt(0, 2, 20).stack, [
		{ source: 0, line: 3, column: 2 },
		{ source: 1, line: 9, column: 4 },
		{ source: 2, line: 0, column: 0 }
	]);
	assert.deepEqual(decoded.traceAt(0, 6, 20).stack, []);
	assert.deepEqual(decoded.traceAt(2, 0, 20).stack, [
		{ source: 1, line: 19, column: 6 }, { source: 2, line: 0, column: 0 }
	]);
	assert.deepEqual(decoded.firstPosition(2), { source: 2, line: 0, column: 0 });
	assert.deepEqual(map().traceAt(0, 0, 20).stack, []);
});

test('accepts maps with no mappings, names, or optional extensions and embedded snapshots', () => {
	assert.equal(map({ mappings: '' }).traceAt(0, 0, 0), undefined);
	assert.deepEqual(map({ sourcesContent: [''] }).sourcesContent, ['']);
	const protectedMap = new DecodedSourceMap('\uFEFF)]}\n{"version":3,"sources":[],"mappings":""}');
	assert.deepEqual(protectedMap.sources, []);
});

test('rejects malformed maps and cyclic or invalid caller graphs', () => {
	for (const encoded of ['!', 'g', 'gggggggggggggA']) { assert.throws(() => decodeVlq(encoded)); }
	assert.throws(() => map({ mappings: 'AA' }), /segment/);
	assert.throws(() => map({ mappings: 'ACAA' }), /out of range/);
	assert.throws(() => map({ mappings: 'DAAA' }), /segment/);
	assert.throws(() => map({ version: 2 }), /revision 3/);
	assert.throws(() => map({ sections: [] }), /flat/);
	assert.throws(() => map({ sourcesContent: [2] }), /sourcesContent/);
	assert.throws(() => map({ x_de_hicknhack_software_callstack: { callers: 'AAAA' } }), /Cyclic/);
	assert.throws(() => map({ x_de_hicknhack_software_callstack: { callers: 'AAAE' } }), /parent/);
	assert.throws(() => map({ x_de_hicknhack_software_callstack: { indices: 'C' } }), /caller index/);
});
