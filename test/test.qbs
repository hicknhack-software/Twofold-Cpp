Project {
    name: "Tests"
    references: [
        "TestTemplateBuilder/TestTemplateBuilder.qbs",
        "TestBenchmark/TestBenchmark.qbs",
        "TestLines/TestLines.qbs"
    ]

    AutotestRunner {}
}
