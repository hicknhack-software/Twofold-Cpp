// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#include "../SourceMapTestSupport.h"
#include <TwofoldGenerator/FileHandlerMemory.h>
#include <TwofoldGenerator/MessageHandlerMemory.h>
#include <TwofoldGenerator/PreparedTemplateBuilder.h>
#include <catch2/generators/catch_generators.hpp>

using namespace smtest;

TEST_CASE("Twofold preparation saves maps and embeds portable template origins", "[sourcemap]") {
    auto files = std::make_shared<Twofold::FileHandlerMemory>(Twofold::FileHandlerMemory::FileMap{
        {"templates/cpp/root.hpp.twofold",
         "#include \"leaf.hpp.twofold\"\nauto run() -> Twofold {\n|hello #{value}\n}\n"},
        {"templates/cpp/leaf.hpp.twofold", "auto leaf() -> Twofold {\n|é😀 value\n}\n"},
    });
    auto messages = std::make_shared<Twofold::MessageHandlerMemory>();
    auto builder = Twofold::PreparedTemplateBuilder{
        {.outputBasePath = "out", .messageHandler = messages, .fileHandler = files, .sourceRoot = "templates"}};
    builder.build("templates/cpp/root.hpp.twofold");
    REQUIRE(messages->messages().empty());
    REQUIRE(files->savedFiles().size() == 4);
    auto saved = std::map<std::string, std::string>{};
    for (auto const& file : files->savedFiles()) saved[file.savePath.generic_string()] = file.generatedContent;
    REQUIRE(saved.contains("out/root.hpp.map"));
    CHECK(saved.at("out/root.hpp").ends_with("//# sourceMappingURL=root.hpp.map\n"));
    CHECK(saved.at("out/leaf.hpp").ends_with("//# sourceMappingURL=leaf.hpp.map\n"));
    CHECK(saved.at("out/root.hpp").contains("template://cpp/root.hpp.twofold"));
    auto map = decode(saved.at("out/root.hpp.map"));
    verifyPortable(map);
    auto position = positionOf(saved.at("out/root.hpp"), "TWOFOLD_EXPR(value)");
    position.column += std::string_view{"TWOFOLD_EXPR("}.size();
    auto origin = SourceMap::getOriginalPositionFromGenerated(map, position);
    CHECK(origin.name.string() == "template://cpp/root.hpp.twofold");
    CHECK(origin.position == SourceMap::Position{3, 10});
    auto trailer = positionOf(saved.at("out/root.hpp"), "//# sourceMappingURL=");
    auto entry = map.findEntryByGenerated(trailer);
    CHECK((!entry || entry->generated.line != trailer.line || !entry->original.isValid()));
}

TEST_CASE("Prepared C++ positions map back to template script, substitutions, and escapes", "[sourcemap]") {
    struct Case {
        std::string_view source;
        std::string_view generatedNeedle;
        std::string_view sourceNeedle;
    };
    // Port the formerly commented-out source-map scenarios to the current C++ API.
    auto data = GENERATE(
        Case{"auto value = test ? 1 : 2;\n", "1 : 2", "1 : 2"},
        Case{"| simple #{test ? 1 : 2}\n", "1 : 2", "1 : 2"},
        Case{"| simple\n|#{\"hello\"}\n", "hello", "hello"},
        Case{"|simple\n", "co_yield TwofoldRuntime::Append{", "|"},
        Case{"|#{test}\n", "co_yield TwofoldRuntime::PushPartIndent{", "#"},
        Case{"|#{test}\n", "), TwofoldRuntime::FilePosition", "}"},
        Case{"|é😀 quote \" after #{42}\n", "after", "after"},
        Case{"|é😀 quote \" after #{42}\n", "\\\"", "\""},
        Case{"|##{escaped}\n", "{escaped}", "{escaped}"});
    auto files = std::make_shared<Twofold::FileHandlerMemory>(Twofold::FileHandlerMemory::FileMap{
        {"templates/case.hpp.twofold", std::string{data.source}},
    });
    auto messages = std::make_shared<Twofold::MessageHandlerMemory>();
    auto builder = Twofold::PreparedTemplateBuilder{
        {.outputBasePath = "out", .messageHandler = messages, .fileHandler = files, .sourceRoot = "templates"}};
    builder.build("templates/case.hpp.twofold");
    REQUIRE(messages->messages().empty());
    REQUIRE(files->savedFiles().size() == 2);
    auto map = decode(files->savedFiles()[0].generatedContent);
    auto const& cpp = files->savedFiles()[1].generatedContent;
    auto origin = SourceMap::getOriginalPositionFromGenerated(map, positionOf(cpp, data.generatedNeedle));
    CAPTURE(data.source, data.generatedNeedle);
    CHECK(origin.name.string() == "template://case.hpp.twofold");
    CHECK(origin.position == positionOf(data.source, data.sourceNeedle));
}
