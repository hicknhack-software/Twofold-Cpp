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
#include "LineProcessor.h"

#include <algorithm>
#include <cassert>
#include <cctype>

namespace Twofold {
namespace intern {

namespace {

auto findProcessor(LineProcessor::Map& map, LineProcessor::Function& fallback, FileLine const& line)
    -> LineProcessor::Function& {
    if (line.firstNonSpace != line.end) {
        auto it = map.find(*line.firstNonSpace);
        if (it != map.end()) return it->second;
    }
    return fallback;
}

} // namespace

LineProcessor::LineProcessor(Map map, Function fallback)
    : m_map{std::move(map)}
    , m_fallback{std::move(fallback)} {
    assert(m_fallback);
}

void LineProcessor::operator()(std::filesystem::path const& name, std::string_view const& text) {
    auto line = FileLine{
        .position = FilePosition{.name = name, .position = {0, 0}},
        .begin = text.begin(),
        .end = {},
        .firstNonSpace = {},
    };
    auto const textEnd = text.end();
    while (line.begin != textEnd) {
        line.position.position.line++;
        line.firstNonSpace = std::find_if_not(
            line.begin, textEnd, [](auto ch) { return std::isspace(static_cast<unsigned char>(ch)) && ch != '\n'; });
        line.end = std::find_if(line.firstNonSpace, textEnd, [](auto ch) { return ch == '\n'; });

        findProcessor(m_map, m_fallback, line)(line);

        if (line.end == textEnd) break; // end of file
        line.begin = line.end + 1;
    }
}

} // namespace intern
} // namespace Twofold
