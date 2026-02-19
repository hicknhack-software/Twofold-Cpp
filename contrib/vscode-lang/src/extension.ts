import * as vscode from 'vscode';
import { TwicefoldDocumentFormatter } from './formatter';

export function activate(context: vscode.ExtensionContext) {
	const twofoldFormatter = new TwicefoldDocumentFormatter();

	// Register formatter for both twofold language IDs
	context.subscriptions.push(
		vscode.languages.registerDocumentFormattingEditProvider(
			'twofold',
			twofoldFormatter
		),
		vscode.languages.registerDocumentFormattingEditProvider(
			'twofold-cpp',
			twofoldFormatter
		)
	);
}

export function deactivate() {}
