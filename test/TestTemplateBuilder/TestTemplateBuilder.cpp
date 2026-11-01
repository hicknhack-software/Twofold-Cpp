/* Twofold-Cpp
 * (C) Copyright 2025 HicknHack Software GmbH
 *
 * The original code can be found at:
 *     https://github.com/hicknhack-software/Twofold-Cpp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "TwofoldGenerator/FileHandlerMemory.h"
#include "TwofoldGenerator/MessageHandlerMemory.h"
#include "TwofoldGenerator/PreparedTemplateBuilder.h"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <format>
#include <iterator>
#include <string>
#include <vector>

using namespace Twofold;

namespace {

struct InterpolationTestData {
    std::string name;
    std::string templateText;
    std::string expectedCpp;
};

auto s_pos = [](int line, int col) {
    return std::format(
        "TwofoldRuntime::FilePosition{{\"template://testTemplate\", {{.line={}, .column={}}}}}", line, col);
};

auto s_newLine = [](int line, int col) {
    return std::format("co_yield TwofoldRuntime::NewLine{{{}}};\n", s_pos(line, col));
};
auto s_output = [](std::string text, int line, int col) {
    return std::format("co_yield TwofoldRuntime::Append{{\"{}\", {}}};\n", text, s_pos(line, col));
};
auto s_indentPart = [](std::string text, int line, int col) {
    return std::format("co_yield TwofoldRuntime::IndentPart{{\"{}\", {}}};\n", text, s_pos(line, col));
};
auto s_expression = [](std::string text, int line, int col) {
    auto pos = s_pos(line, col);
    return std::format(
        "co_yield TwofoldRuntime::PushPartIndent{{{0}}};"
        "co_yield TwofoldRuntime::AppendExpression{{TWOFOLD_EXPR({1}), {0}}};"
        "co_yield TwofoldRuntime::PopPartIndent{{}};\n",
        pos,
        text);
};

auto s_inlineCall = [](int line, int col) {
    return std::format(
        "co_yield TwofoldRuntime::PushCaller{{{}}};co_await test();co_yield TwofoldRuntime::PopCaller{{}};\n",
        s_pos(line, col));
};

} // namespace

TEST_CASE("TestTemplateBuilder Interpolation", "[interpolation]") {
    auto data = GENERATE(
        InterpolationTestData{"empty", "|", s_indentPart("", 1, 2) + s_newLine(1, 2)},
        InterpolationTestData{
            "simple", "|simple ", s_indentPart("", 1, 2) + s_output("simple ", 1, 2) + s_newLine(1, 9)},
        InterpolationTestData{
            "indentation space", " \t| simple", s_indentPart(" ", 1, 4) + s_output("simple", 1, 5) + s_newLine(1, 11)},
        InterpolationTestData{
            "indentation tab", " |\tsimple", s_indentPart("\t", 1, 3) + s_output("simple", 1, 4) + s_newLine(1, 10)},
        InterpolationTestData{
            "no expr", "|#(test", s_indentPart("", 1, 2) + s_output("#(test", 1, 2) + s_newLine(1, 8)},
        InterpolationTestData{
            "escaped expr",
            "|##{test",
            s_indentPart("", 1, 2) + s_output("#", 1, 2) + s_output("{test", 1, 4) + s_newLine(1, 9)},
        InterpolationTestData{
            "single expr 1", "|#{test}", s_indentPart("", 1, 2) + s_expression("test", 1, 4) + s_newLine(1, 9)},
        InterpolationTestData{
            "single expr 2",
            "|#{test}}",
            s_indentPart("", 1, 2) + s_expression("test", 1, 4) + s_output("}", 1, 9) + s_newLine(1, 10)},
        InterpolationTestData{
            "inline expr 1",
            "|simple #{test.prop} expr",
            s_indentPart("", 1, 2) + s_output("simple ", 1, 2) + s_expression("test.prop", 1, 11) +
                s_output(" expr", 1, 21) + s_newLine(1, 26)},
        InterpolationTestData{
            "logic expr",
            "|#{ (i==0 ? ':' : \",\") }",
            s_indentPart("", 1, 2) + s_expression(" (i==0 ? ':' : \",\") ", 1, 4) + s_newLine(1, 25)},
        InterpolationTestData{
            "escaped expr 1",
            "|escape ##{test} expr",
            s_indentPart("", 1, 2) + s_output("escape #", 1, 2) + s_output("{test} expr", 1, 11) + s_newLine(1, 22)},
        InterpolationTestData{
            "escaped expr 2",
            "|###{test}",
            s_indentPart("", 1, 2) + s_output("#", 1, 2) + s_expression("test", 1, 6) + s_newLine(1, 11)},
        InterpolationTestData{
            "string expr 1", "|#{\"}\"}", s_indentPart("", 1, 2) + s_expression("\"}\"", 1, 4) + s_newLine(1, 8)},
        InterpolationTestData{
            "string expr 2",
            "|#{'\\'\\\"}'}",
            s_indentPart("", 1, 2) + s_expression("'\\'\\\"}'", 1, 4) + s_newLine(1, 12)},
        InterpolationTestData{
            "nested expr",
            "|#{'#{hello}'}",
            s_indentPart("", 1, 2) + s_expression("'#{hello}'", 1, 4) + s_newLine(1, 15)},
        InterpolationTestData{
            "call expr shorthand",
            "|#{co_await test()}",
            s_indentPart("", 1, 2) + s_inlineCall(1, 4) + s_newLine(1, 20)},
        InterpolationTestData{
            "call expr shorthand with text",
            "|prefix #{co_await test()} suffix",
            s_indentPart("", 1, 2) + s_output("prefix ", 1, 2) + s_inlineCall(1, 11) + s_output(" suffix", 1, 27) +
                s_newLine(1, 34)});

    SECTION(data.name) {
        auto messageHandler = std::make_shared<MessageHandlerMemory>();
        auto fileHandler =
            std::make_shared<FileHandlerMemory>(FileHandlerMemory::FileMap{{"testTemplate", data.templateText}});

        auto builder = PreparedTemplateBuilder{{
            .outputBasePath = {},
            .messageHandler = messageHandler,
            .fileHandler = fileHandler,
        }};
        builder.build("testTemplate");

        CAPTURE(data.name);
        CHECK(messageHandler->messages().empty());
        REQUIRE(fileHandler->savedFiles().size() == 2);
        CHECK(
            fileHandler->savedFiles()[1].generatedContent ==
            data.expectedCpp + "\n//# sourceMappingURL=testTemplate.map\n");
    }
}
