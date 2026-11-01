// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#include "../SourceMapTestSupport.h"
#include "SourceMapFixture.hpp"
#include <TwofoldRuntime/Twofold.h>

using namespace smtest;

TEST_CASE("Twofold runtime maps multiline expressions and retains sibling call stacks", "[sourcemap]") {
    auto builder = TwofoldRuntime::TargetBuilder{};
    auto result = sourceMapFixture();
    result.setBuilder(&builder);
    result.run();
    auto output = builder.build();
    CHECK(
        output.text ==
        "  leaf: first\n  second\ninline leaf: inside\n suffix\nleaf: sibling\n#{escaped} helper\né😀 end\ntail");
    verifyPortable(output.sourceMap);
    auto json = SourceMap::RevisionThree{};
    json.encodeMappings(output.sourceMap);
    auto map = decode(json.toJson());
    verifyPortable(map);
    for (auto needle : {"first", "second", "inside", "sibling"}) {
        auto position = positionOf(output.text, needle);
        auto entry = map.findEntryByGenerated(position);
        REQUIRE(entry);
        auto origin = SourceMap::getOriginalPositionFromGenerated(map, position);
        CHECK(origin.name.string() == "template://SourceMapFixture.hpp.twofold");
        CHECK(origin.position == SourceMap::Position{10, 18});
        auto stack = SourceMap::buildCallerStack(map, entry);
        REQUIRE(stack.size() == 1);
        auto expectedLine = std::string_view{needle} == "inside" ? 15 : std::string_view{needle} == "sibling" ? 16 : 14;
        CHECK(stack.front().position.line == expectedLine);
    }
    auto unicode = SourceMap::getOriginalPositionFromGenerated(map, positionOf(output.text, "end"));
    CHECK(unicode.position == SourceMap::Position{18, 14});
    auto escaped = SourceMap::getOriginalPositionFromGenerated(map, positionOf(output.text, "{escaped}"));
    CHECK(escaped.position == SourceMap::Position{17, 12});
    auto helperEntry = map.findEntryByGenerated(positionOf(output.text, "helper"));
    REQUIRE(helperEntry);
    CHECK(SourceMap::buildCallerStack(map, helperEntry).empty());
}

TEST_CASE("Nested template callers and input scopes unwind in order", "[sourcemap]") {
    auto builder = TwofoldRuntime::TargetBuilder{};
    auto result = sourceMapDeep();
    result.setBuilder(&builder);
    result.run();
    auto output = builder.build();
    auto entry = output.sourceMap.findEntryByGenerated(positionOf(output.text, "first"));
    REQUIRE(entry);
    auto stack = SourceMap::buildCallerStack(output.sourceMap, entry);
    REQUIRE(stack.size() == 2);
    CHECK(stack[0].position.line == 14);
    CHECK(stack[1].position.line == 23);

    auto json = SourceMap::RevisionThree{};
    json.encodeMappings(output.sourceMap);
    auto decoded = decode(json.toJson());
    auto decodedEntry = decoded.findEntryByGenerated(positionOf(output.text, "first"));
    REQUIRE(decodedEntry);
    CHECK(SourceMap::buildCallerStack(decoded, decodedEntry) == stack);
    CHECK(SourceMap::get<ExtCaller>(decoded.data()) == SourceMap::get<ExtCaller>(output.sourceMap.data()));

    auto scopedBuilder = TwofoldRuntime::TargetBuilder{};
    scopedBuilder.setSourceResolver([](char const* location) -> TwofoldRuntime::FilePosition {
        return {std::string{"input://"} + location, {1, 1}};
    });
    auto render = []() -> Twofold {
        auto outer = co_await TwofoldRuntime::SourceScope{"outer"};
        {
            auto inner = co_await TwofoldRuntime::SourceScope{"inner"};
            co_yield TwofoldRuntime::Append{"a", {"template://test", {1, 1}}};
        }
        co_yield TwofoldRuntime::Append{"b", {"template://test", {1, 2}}};
    };
    auto scoped = render();
    scoped.setBuilder(&scopedBuilder);
    scoped.run();
    auto scopedOutput = scopedBuilder.build();
    auto innerStack =
        SourceMap::buildCallerStack(scopedOutput.sourceMap, scopedOutput.sourceMap.findEntryByGenerated({1, 1}));
    REQUIRE(innerStack.size() == 2);
    CHECK(innerStack[0].name.string() == "input://inner");
    CHECK(innerStack[1].name.string() == "input://outer");
    auto outerStack =
        SourceMap::buildCallerStack(scopedOutput.sourceMap, scopedOutput.sourceMap.findEntryByGenerated({1, 2}));
    REQUIRE(outerStack.size() == 1);
    CHECK(outerStack[0].name.string() == "input://outer");

    auto fail = []() -> Twofold {
        auto scope = co_await TwofoldRuntime::SourceScope{"failed"};
        throw std::runtime_error("failed template");
        co_return;
    };
    auto failed = fail();
    failed.setBuilder(&scopedBuilder);
    CHECK_THROWS_AS(failed.run(), std::runtime_error);
    scopedBuilder.append("c", {"template://test", {1, 3}});
    auto after = scopedBuilder.build();
    CHECK(SourceMap::buildCallerStack(after.sourceMap, after.sourceMap.findEntryByGenerated({1, 3})).empty());
}

TEST_CASE("Repeated input scopes preserve template calls without duplicating the input origin", "[sourcemap]") {
    auto builder = TwofoldRuntime::TargetBuilder{};
    builder.setSourceResolver([](char const* location) -> TwofoldRuntime::FilePosition {
        return {std::string{"input://"} + location, {1, 1}};
    });
    auto child = []() -> Twofold {
        auto sameNode = co_await TwofoldRuntime::SourceScope{"node"};
        co_yield TwofoldRuntime::Append{"a", {"template://child", {1, 1}}};
        {
            auto differentNode = co_await TwofoldRuntime::SourceScope{"nested"};
            co_yield TwofoldRuntime::Append{"b", {"template://child", {2, 1}}};
        }
        co_yield TwofoldRuntime::Append{"c", {"template://child", {3, 1}}};
    };
    auto render = [&]() -> Twofold {
        auto outer = co_await TwofoldRuntime::SourceScope{"node"};
        co_yield TwofoldRuntime::PushCaller{{"template://parent", {4, 5}}};
        co_await child();
        co_yield TwofoldRuntime::PopCaller{};
        co_yield TwofoldRuntime::Append{"d", {"template://parent", {6, 1}}};
    };
    auto result = render();
    result.setBuilder(&builder);
    result.run();
    auto output = builder.build();
    auto const expected = SourceMap::CallerStack{
        {"template://parent", {4, 5}},
        {"input://node", {1, 1}},
    };
    for (auto column : {1, 3}) {
        auto entry = output.sourceMap.findEntryByGenerated({1, column});
        REQUIRE(entry);
        CHECK(SourceMap::buildCallerStack(output.sourceMap, entry) == expected);
    }
    auto nestedExpected = expected;
    nestedExpected.insert(nestedExpected.begin(), {"input://nested", {1, 1}});
    CHECK(
        SourceMap::buildCallerStack(output.sourceMap, output.sourceMap.findEntryByGenerated({1, 2})) == nestedExpected);
    CHECK(
        SourceMap::buildCallerStack(output.sourceMap, output.sourceMap.findEntryByGenerated({1, 4})) ==
        SourceMap::CallerStack{{"input://node", {1, 1}}});
    auto json = SourceMap::RevisionThree{};
    json.encodeMappings(output.sourceMap);
    auto decoded = decode(json.toJson());
    CHECK(SourceMap::get<ExtCaller>(decoded.data()) == SourceMap::get<ExtCaller>(output.sourceMap.data()));
}
