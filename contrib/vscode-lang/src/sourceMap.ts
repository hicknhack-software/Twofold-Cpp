// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

/** SourceMap-Cpp uses the revision 3 mappings plus two optional VLQ extensions.
 * All positions exposed here are zero based UTF-16 positions, as in VS Code.
 */
export interface SourcePosition {
	source: number;
	line: number;
	column: number;
}

export interface Segment {
	column: number;
	original?: SourcePosition;
	interpolate: boolean;
	caller: number;
}

interface Caller extends SourcePosition { parent: number; }

export interface Trace {
	original: SourcePosition;
	segment: Segment;
	endColumn: number;
	stack: SourcePosition[];
}

const alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';

export function decodeVlq(segment: string): number[] {
	const result: number[] = [];
	let value = 0;
	let scale = 1;
	for (const character of segment) {
		const digit = alphabet.indexOf(character);
		if (digit < 0) { throw new Error('Invalid base64 VLQ character.'); }
		value += (digit % 32) * scale;
		if (!Number.isSafeInteger(value)) { throw new Error('VLQ value exceeds integer precision.'); }
		if (digit < 32) {
			result.push(value % 2 ? -Math.floor(value / 2) : value / 2);
			value = 0;
			scale = 1;
		} else {
			scale *= 32;
			if (scale > Number.MAX_SAFE_INTEGER) { throw new Error('VLQ value is too long.'); }
		}
	}
	if (scale !== 1) { throw new Error('Truncated base64 VLQ value.'); }
	return result;
}

export function sourceMapReference(text: string): string | undefined {
	// Only standalone comment directives count; use the last directive in a file.
	const pattern = /^[\t ]*(?:\/\/[#@][\t ]*sourceMappingURL[\t ]*=[\t ]*(\S+)|\/\*[#@][\t ]*sourceMappingURL[\t ]*=[\t ]*(\S+?)[\t ]*\*\/|<!--[\t ]*[#@][\t ]*sourceMappingURL[\t ]*=[\t ]*(\S+?)[\t ]*-->)[\t ]*\r?$/gm;
	let reference: string | undefined;
	for (const match of text.matchAll(pattern)) { reference = match[1] ?? match[2] ?? match[3]; }
	return reference;
}

export function inlineSourceMap(reference: string): string | undefined {
	if (!reference.startsWith('data:')) { return undefined; }
	const match = /^data:application\/json(?:;charset=[\w-]+)?(;base64)?,(.*)$/i.exec(reference);
	if (!match) { throw new Error('Unsupported inline source map data URL.'); }
	return match[1] ? Buffer.from(match[2], 'base64').toString('utf8') : decodeURIComponent(match[2]);
}

function stringField(object: Record<string, unknown>, name: string, fallback = ''): string {
	const value = object[name] ?? fallback;
	if (typeof value !== 'string') { throw new Error(`Invalid source map ${name}.`); }
	return value;
}

function objectField(value: unknown): Record<string, unknown> {
	if (!value || typeof value !== 'object' || Array.isArray(value)) { throw new Error('Invalid source map object.'); }
	return value as Record<string, unknown>;
}

export class DecodedSourceMap {
	readonly sources: string[];
	readonly sourcesContent: (string | null)[];
	readonly sourceRoot: string;
	readonly lines: Segment[][];
	private readonly callers: Caller[] = [];
	private readonly firstPositions: (SourcePosition | undefined)[] = [];

	constructor(text: string) {
		const data = objectField(JSON.parse(text.replace(/^\uFEFF/, '').replace(/^\)\]\}[^\n]*\n/, '')));
		if (data.version !== 3 || data.sections !== undefined) {
			throw new Error('Expected a flat revision 3 source map.');
		}
		if (!Array.isArray(data.sources) || !data.sources.every(source => typeof source === 'string')) {
			throw new Error('Invalid source map sources.');
		}
		this.sources = data.sources;
		this.sourceRoot = stringField(data, 'sourceRoot');
		const contents = data.sourcesContent ?? [];
		if (!Array.isArray(contents) || !contents.every(content => content === null || typeof content === 'string')) {
			throw new Error('Invalid source map sourcesContent.');
		}
		this.sourcesContent = contents;
		const callstack = data.x_de_hicknhack_software_callstack === undefined
			? {} : objectField(data.x_de_hicknhack_software_callstack);
		let source = 0, line = 0, column = 0, parent = 0;
		for (const encoded of stringField(callstack, 'callers').split(';')) {
			if (!encoded) { continue; }
			const values = decodeVlq(encoded);
			if (values.length !== 3 && values.length !== 4) { throw new Error('Invalid caller entry.'); }
			source += values[0]; line += values[1]; column += values[2];
			this.validatePosition(source, line, column);
			// Missing parents do not reset the delta accumulator (SourceMap-Cpp).
			this.callers.push({ source, line, column, parent: values.length === 4 ? (parent += values[3]) : -1 });
		}
		for (const caller of this.callers) {
			if (caller.parent < -1 || caller.parent >= this.callers.length) { throw new Error('Invalid caller parent.'); }
		}
		// Check the graph once, in linear time, including otherwise unused callers.
		const visited = new Set<number>();
		this.callers.forEach((_, index) => {
			const path = new Set<number>();
			while (index !== -1 && !visited.has(index)) {
				if (path.has(index)) { throw new Error('Cyclic source map caller stack.'); }
				path.add(index); index = this.callers[index].parent;
			}
			for (const member of path) { visited.add(member); }
		});
		const interpolationLines = stringField(data, 'x_de_hicknhack_software_column_interpolation').split(';');
		const indexLines = stringField(callstack, 'indices').split(';');
		let lastCaller = 0;
		source = 0; line = 0; column = 0;
		this.lines = stringField(data, 'mappings').split(';').map((encodedLine, lineIndex) => {
			let generatedColumn = 0;
			const flags = (interpolationLines[lineIndex] ?? '').split(',');
			const indices = (indexLines[lineIndex] ?? '').split(',');
			if (!encodedLine) { return []; }
			return encodedLine.split(',').map((encoded, segmentIndex) => {
				const values = decodeVlq(encoded);
				if (![1, 4, 5].includes(values.length) || values[0] < 0) { throw new Error('Invalid mapping segment.'); }
				generatedColumn += values[0];
				let original: SourcePosition | undefined;
				if (values.length > 1) {
					source += values[1]; line += values[2]; column += values[3];
					this.validatePosition(source, line, column);
					original = { source, line, column };
					this.firstPositions[source] ??= original;
				}
				const flag = decodeVlq(flags[segmentIndex] ?? '');
				if (flag.length > 1 || (flag.length && flag[0] !== 0 && flag[0] !== 1)) {
					throw new Error('Invalid column interpolation flag.');
				}
				const index = decodeVlq(indices[segmentIndex] ?? '');
				if (index.length > 1) { throw new Error('Invalid caller index.'); }
				const caller = index.length ? (lastCaller += index[0]) : -1;
				if (caller < -1 || caller >= this.callers.length) { throw new Error('Invalid caller index.'); }
				return { column: generatedColumn, original, interpolate: flag[0] === 1, caller };
			});
		});
		for (const { source, line, column } of this.callers) {
			this.firstPositions[source] ??= { source, line, column };
		}
	}

	private validatePosition(source: number, line: number, column: number): void {
		if (source < 0 || source >= this.sources.length || line < 0 || column < 0) {
			throw new Error('Source map position is out of range.');
		}
	}

	traceAt(line: number, column: number, lineLength: number): Trace | undefined {
		const segments = this.lines[line] ?? [];
		let low = 0, high = segments.length;
		while (low < high) {
			const middle = Math.floor((low + high) / 2);
			if (segments[middle].column <= column) { low = middle + 1; } else { high = middle; }
		}
		const segment = segments[low - 1];
		if (!segment?.original) { return undefined; }
		const original = { ...segment.original };
		if (segment.interpolate) { original.column += column - segment.column; }
		const stack: SourcePosition[] = [];
		let caller = segment.caller;
		while (caller !== -1) {
			const { parent, ...position } = this.callers[caller];
			stack.push(position); caller = parent;
		}
		return { original, segment, endColumn: segments[low]?.column ?? lineLength, stack };
	}

	/** Includes sources referenced only by caller frames. */
	firstPosition(source: number): SourcePosition {
		return this.firstPositions[source] ?? { source, line: 0, column: 0 };
	}
}
