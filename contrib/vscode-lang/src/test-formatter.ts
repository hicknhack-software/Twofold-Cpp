import * as fs from 'fs';
import { TwofoldFormatter } from './formatter.js';

async function main() {
	const args = process.argv.slice(2);

	if (args.length < 1) {
		console.error('Usage: node test-formatter.js <clang-format-path> [file-path]');
		console.error('  clang-format-path: Path to clang-format executable');
		console.error('  file-path: Optional. If not provided, reads from stdin');
		process.exit(1);
	}

	const clangFormatPath = args[0];
	let content: string;

	if (args.length >= 2) {
		content = fs.readFileSync(args[1], 'utf-8');
		console.log(`Formatting file: ${args[1]}`);
	} else {
		content = fs.readFileSync(0, 'utf-8'); // Read from stdin
		console.log('Formatting from stdin...');
	}

	console.log('\n=== ORIGINAL CONTENT ===');
	console.log(content);
	console.log('=== END ORIGINAL ===\n');

	try {
		const formatter = await TwofoldFormatter.createWithPath(clangFormatPath, 0);
		const formatted = await formatter.formatContent(content);

		console.log('\n=== FORMATTED CONTENT ===');
		console.log(formatted);
		console.log('=== END FORMATTED ===\n');

		const changed = content !== formatted;
		console.log(`Changed: ${changed}`);

	} catch (error) {
		console.error('Error:', error);
		process.exit(1);
	}
}

main();
