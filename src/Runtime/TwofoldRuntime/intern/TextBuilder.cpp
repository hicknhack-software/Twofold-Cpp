// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#include "TextBuilder.h"

#include <SourceMap/TextPosition.h>
#include <algorithm>

namespace TwofoldRuntime::intern {

auto TextBuilder::column() const -> int {
    if (0 == m_column) return 1 + SourceMap::utf16Length(m_indentation);
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
            if (0 == column) column = 1 + SourceMap::utf16Length(m_indentation);
            column += SourceMap::utf16Length(std::string_view{it, end});
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

} // namespace TwofoldRuntime::intern
