// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

const { test } = require('node:test');
const { verifyFixtures } = require('./realFixtures.cjs');

test('navigates maps emitted by the actual Twofold and Protlr3 generators', {
	skip: process.env.TWOFOLD_REAL_FIXTURES ? false : 'Set TWOFOLD_REAL_FIXTURES to generated fixture directory'
}, () => { verifyFixtures(process.env.TWOFOLD_REAL_FIXTURES); });
