// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

CppApplication {
    name: "TestRuntimeSourceMaps"
    consoleApplication: true
    type: base.concat(["autotest"])

    Depends { name: "TwofoldCodeGen" }
    Depends { name: "TwofoldRuntimeLibrary" }
    Depends { name: "catch2-with-main" }

    files: [
        "TestRuntimeSourceMaps.cpp",
        "SourceMapFixture.hpp.twofold",
        "../SourceMapTestSupport.h",
    ]
}
