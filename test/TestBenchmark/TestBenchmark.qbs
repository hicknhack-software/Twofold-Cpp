CppApplication {
    name: "TestBenchmark"

    consoleApplication: true
    cpp.cxxLanguageVersion: "c++23"

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
