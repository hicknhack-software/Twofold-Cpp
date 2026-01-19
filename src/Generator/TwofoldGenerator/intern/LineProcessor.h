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
