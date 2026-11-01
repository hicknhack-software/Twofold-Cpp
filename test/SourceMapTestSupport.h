// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <SourceMap/Extension/Caller.h>
#include <SourceMap/Extension/Interpolation.h>
#include <SourceMap/RevisionThree.h>
#include <SourceMap/TextPosition.h>
#include <catch2/catch_test_macros.hpp>
using Mapping = SourceMap::Mapping<SourceMap::Extension::Interpolation, SourceMap::Extension::Caller>;
using ExtCaller = SourceMap::Extension::Caller;
using ExtInterpolation = SourceMap::Extension::Interpolation;

namespace smtest {
namespace {
auto positionOf(std::string_view content, std::string_view needle) -> SourceMap::Position {
    auto offset = content.find(needle);
    REQUIRE(offset != content.npos);
    auto before = content.substr(0, offset);
    auto last = before.rfind('\n');
    return {
        1 + static_cast<int>(std::ranges::count(before, '\n')),
        1 + SourceMap::utf16Length(before.substr(last == before.npos ? 0 : last + 1))};
}

auto decode(std::string const& json) -> Mapping {
    return Mapping{SourceMap::RevisionThree::fromJson(json).decodedMappings<Mapping::Data>()};
}

auto verifyPortable(Mapping const& mapping) -> void {
    REQUIRE_FALSE(mapping.data().entries.empty());
    for (auto const& source : mapping.originalNames()) {
        auto name = source.string();
        CHECK((name.starts_with("template://") || name.starts_with("input://")));
        CHECK(name.find("/../") == name.npos);
    }
    auto const& callers = SourceMap::get<ExtCaller>(mapping.data());
    for (auto const& entry : mapping.data().entries) {
        auto index = SourceMap::get<ExtCaller>(entry);
        auto steps = size_t{};
        while (index.value != -1) {
            REQUIRE(index.value >= 0);
            REQUIRE(static_cast<size_t>(index.value) < callers.size());
            REQUIRE(++steps <= callers.size());
            index = callers[index.value].parentIndex;
        }
    }
}
} // namespace

} // namespace smtest
