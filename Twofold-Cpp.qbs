import qbs.FileInfo

Project {
    name: "Twofold-Cpp"

    property string version: "2.0.0"
    property string configProductName: "TwofoldConfig"
    property bool enableQbsImports: (sourceDirectory === path)
    property bool enableTests: (sourceDirectory === path)
    property bool enableExamples: (sourceDirectory === path)
    property bool enableThirdParty: true

    minimumQbsVersion: "3.1"
    qbsSearchPaths: enableQbsImports ? ["qbs"] : []

    Product {
        name: "TwofoldConfig"
        condition: configProductName === "TwofoldConfig"

        Export {
            cpp.cxxLanguageVersion: "c++23"

            Depends { name: "cpp" }
        }
    }
    StaticLibrary {
        name: "TwofoldGeneratorLibrary"
        version: parent.version

        cpp.includePaths: "src/Generator"

        Depends { name: "cpp" }
        Depends { name: "SourceMapLibrary" }
        Export {
            cpp.includePaths: FileInfo.joinPaths(exportingProduct.sourceDirectory, "src", "Generator")

            Depends { name: "cpp" }
            Depends { name: "SourceMapLibrary" }
        }
        Group {
            name: "sources"
            prefix: "src/Generator/TwofoldGenerator/"
            files: [
                "FileHandler.h",
                "FileHandlerMemory.h",
                "FileHandlerSimple.cpp",
                "FileHandlerSimple.h",
                "MessageHandler.h",
                "MessageHandlerChain.cpp",
                "MessageHandlerChain.h",
                "MessageHandlerMemory.h",
                "MessageHandlerOstream.h",
                "PreparedTemplateBuilder.cpp",
                "PreparedTemplateBuilder.h",
                "SourceMap.h",
                "intern/CodeBuilder.cpp",
                "intern/CodeBuilder.h",
                "intern/Command/Passthrough.cpp",
                "intern/Command/Passthrough.h",
                "intern/Line/Passthrough.cpp",
                "intern/Line/Passthrough.h",
                "intern/LineProcessor.cpp",
                "intern/LineProcessor.h",
                "intern/SourceMapTextBuilder.cpp",
                "intern/SourceMapTextBuilder.h",
                "intern/TextBuilder.cpp",
                "intern/TextBuilder.h",
                "intern/Command/Include.cpp",
                "intern/Command/Include.h",
                "intern/Command/Missing.cpp",
                "intern/Command/Missing.h",
                "intern/Cpp/BraceCounter.cpp",
                "intern/Cpp/BraceCounter.h",
                "intern/Line/Call.cpp",
                "intern/Line/Call.h",
                "intern/Line/Command.cpp",
                "intern/Line/Command.h",
                "intern/Line/InterpolateLine.cpp",
                "intern/Line/InterpolateLine.h",
                "intern/Line/Interpolation.cpp",
                "intern/Line/Interpolation.h",
            ]
        }
    }
    Application {
        name: "TwofoldGenerator"
        targetName: "TwofoldGenerator"
        version: parent.version
        type: base.concat(["twofold-generator"])

        Group {
            name: "sources"
            prefix: "src/Generator/"
            files: [
                "main.cpp"
            ]
        }
        Group {
            name: "binaries"
            fileTagsFilter: "application"
            fileTags: "twofold-generator"
        }
        Depends { name: "TwofoldGeneratorLibrary" }
    }
    Product {
        name: "TwofoldCodeGen"

        Export {
            cpp.includePaths: [ FileInfo.joinPaths(importingProduct.buildDirectory, "twofold-generated") ]

            additionalProductTypes: ["hpp", "cpp"]

            FileTagger {
                patterns: ["*.hpp.twofold", "*.hxx.twofold", "*.hh.twofold", "*.h.twofold"]
                fileTags: ["hpp-twofold"]
            }
            FileTagger {
                patterns: ["*.cpp.twofold", "*.cxx.twofold", "*.cc.twofold"]
                fileTags: ["cpp-twofold"]
            }
            Rule {
                inputs: ["hpp-twofold", "cpp-twofold"]
                explicitlyDependsOnFromDependencies: ["twofold-generator"]
                outputFileTags: ["hpp", "cpp"]
                outputArtifacts: {
                    var artifacts = [];
                    // var executablePath = inputs["application"][0].filePath; // TODO: use --preview to get all generated files
                    var tags = ["hpp-twofold", "cpp-twofold"];
                    for (var t = 0; t < tags.length; t++) {
                        var inTag = tags[t];
                        if (!inputs[inTag]) continue;
                        for (var i = 0; i < inputs[inTag].length; i++) {
                            var input = inputs[inTag][i];
                            artifacts.push({
                                filePath: FileInfo.joinPaths(product.buildDirectory, "twofold-generated", input.completeBaseName),
                                fileTags: [inTag.substring(0, 3)]
                            });
                        }
                    }
                    return artifacts;
                }
                prepare: {
                    var cmds = [];
                    var executablePath = explicitlyDependsOn["twofold-generator"][0].filePath;
                    var tags = ["hpp-twofold", "cpp-twofold"];
                    for (var t = 0; t < tags.length; t++) {
                        var inTag = tags[t];
                        if (!inputs[inTag]) continue;
                        for (var i = 0; i < inputs[inTag].length; i++) {
                            var inputFilePath = inputs[inTag][i].filePath;
                            var args = ["-i", inputFilePath, "-o", FileInfo.joinPaths(product.buildDirectory, "twofold-generated")];
                            var cmd = new Command(executablePath, args);
                            cmd.description = "folding " + FileInfo.relativePath(product.sourceDirectory, inputFilePath);
                            cmd.highlight = "codegen";
                            cmds.push(cmd);
                        }
                    }
                    return cmds;
                }
            }
            Depends { name: "TwofoldGenerator" }
            Depends { name: "TwofoldRuntimeLibrary" }
            Depends { name: "cpp" }
        }
    }
    StaticLibrary {
        name: "TwofoldRuntimeLibrary"
        version: parent.version

        cpp.includePaths: "src/Runtime"

        Depends { name: "cpp" }
        Depends { name: "SourceMapLibrary" }
        Export {
            cpp.includePaths: FileInfo.joinPaths(exportingProduct.sourceDirectory, "/src/Runtime")

            Depends { name: "cpp" }
            Depends { name: "SourceMapLibrary" }
        }

        Group {
            name: "sources"
            prefix: "src/Runtime/TwofoldRuntime/"
            files: [
                "SourceMap.h",
                "TargetBuilder.cpp",
                "TargetBuilder.h",
                "Twofold.h",
                "intern/SourceMapTextBuilder.cpp",
                "intern/SourceMapTextBuilder.h",
                "intern/TextBuilder.cpp",
                "intern/TextBuilder.h",
            ]
        }
    }
    SubProject {
        condition: parent.enableTests
        filePath: "test/test.qbs"
    }
    SubProject {
        condition: parent.enableExamples
        filePath: "example/example.qbs"
    }
    SubProject {
        condition: parent.enableThirdParty
        filePath: "third_party/third_party.qbs"
    }
    Product {
        name: "[fwofold other files]"
        files: [
            ".clang-format",
            "CHANGES",
            "LICENSE",
            "NOTICE",
            "README.md",
        ]
    }
}
