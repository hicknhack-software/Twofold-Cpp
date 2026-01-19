CppApplication {
    name: "BasicUsage"
    consoleApplication: true

    Depends { name: "TwofoldCodeGen" }
    Depends { name: "TwofoldGenerator" }

    files: [
        "generate.hpp.twofold",
        "main.cpp",
    ]
    Group {
        name: "included"
        fileTags: []
        files: [
            "included.hpp.twofold",
        ]
    }
}
