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
