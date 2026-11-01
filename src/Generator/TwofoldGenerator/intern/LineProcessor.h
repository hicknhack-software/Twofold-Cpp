// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "TwofoldGenerator/SourceMap.h"

#include <filesystem>
#include <functional>
#include <map>
#include <string_view>

namespace Twofold {
namespace intern {

/// @brief Parameter structure for line processor functions
struct FileLine {
    FilePosition position;
    std::string_view::iterator begin;
    std::string_view::iterator end;
    std::string_view::iterator firstNonSpace;
};

/// @brief processes lines based on the first character
/// functor service
class LineProcessor {
public:
    using Function = std::move_only_function<void(FileLine const&)>;
    using Map = std::map<char, Function>;

public:
    LineProcessor(Map map, Function fallback);

    void operator()(std::filesystem::path const& name, std::string_view const& text);

private:
    Map m_map;
    Function m_fallback;
};

} // namespace intern
} // namespace Twofold
