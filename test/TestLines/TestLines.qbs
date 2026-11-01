// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

CppApplication {
    name: "TestLines"

    consoleApplication: true
    type: base.concat(["autotest"])

    Depends { name: "TwofoldGeneratorLibrary" }
    Depends { name: "catch2-with-main" }

    files: [
        "TestLines.cpp"
    ]
}
