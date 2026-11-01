// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

CppApplication {
    name: "TestBenchmark"
    consoleApplication: true
    files: [
        "TestBenchmark.cpp",
        "benchmark.hpp.twofold",
    ]

    Group {
        name: "included"
        fileTags: []
        files: [
            "lib.hpp.twofold",
        ]
    }
    Depends { name: "catch2-with-main" }
    Depends { name: "TwofoldCodeGen" }
    Depends { name: "TwofoldGenerator" }
}
