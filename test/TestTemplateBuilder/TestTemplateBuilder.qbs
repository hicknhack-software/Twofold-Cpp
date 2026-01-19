CppApplication {
    name: "TestTemplateBuilder"

    consoleApplication: true
    type: base.concat(["autotest"])

    Depends { name: "TwofoldGeneratorLibrary" }
    Depends { name: "catch2-with-main" }

    files: [
        "TestTemplateBuilder.cpp"
    ]
}
