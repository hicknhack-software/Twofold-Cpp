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
#include "TextBuilder.h"

#include <algorithm>

namespace Twofold::intern {

auto TextBuilder::column() const -> int {
    if (0 == m_column) return 1 + m_indentation.size();
    return m_column;
}

auto TextBuilder::operator<<(std::string_view text) -> TextBuilder& {
    auto it = text.begin();
    auto const end = text.end();
    if (it == end) return *this; // empty text does not trigger indentation
    auto column = m_column;
    if (0 == column) m_buffer += m_indentation;
    while (true) {
        auto line_end = std::find(it, end, '\n');
        if (line_end != end) {
            ++m_line;
            column = 0;
            ++line_end;
        }
        else {
            if (0 == column) column = 1 + m_indentation.size();
            column += std::distance(it, end);
        }
        m_buffer += std::string_view{it, line_end};
        if (line_end == end) break; // end of text reached
        it = line_end;
        m_buffer += m_indentation;
    }
    m_column = column;
    return *this;
}

auto TextBuilder::operator<<(NewLine) -> TextBuilder& {
    ++m_line;
    m_column = 0;
    m_buffer += '\n';
    return *this;
}

} // namespace Twofold::intern
