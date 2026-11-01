// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

Project {
    name: "Tests"
    references: [
        "TestTemplateBuilder/TestTemplateBuilder.qbs",
        "TestBenchmark/TestBenchmark.qbs",
        "TestLines/TestLines.qbs",
        "TestRuntimeSourceMaps/TestRuntimeSourceMaps.qbs"
    ]

    AutotestRunner {}
}
