// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

import * as vscode from 'vscode';
import { TwicefoldDocumentFormatter } from './formatter';
import { registerSourceMapNavigation } from './sourceMapNavigation';

export function activate(context: vscode.ExtensionContext) {
	const twofoldFormatter = new TwicefoldDocumentFormatter();
	registerSourceMapNavigation(context);

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
