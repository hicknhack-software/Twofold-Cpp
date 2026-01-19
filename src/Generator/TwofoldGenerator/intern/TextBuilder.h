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
#include <string>
#include <string_view>

namespace Twofold::intern {

/// @brief empty structure marking new lines
struct NewLine {};

/// @brief text between two iterators
struct TextSpan {
    inline TextSpan(std::string_view::iterator begin, std::string_view::iterator end)
        : begin{begin}
        , end{end} {}

    /// @brief construct from a std::string
    /// @param text has to live at least as long as the TextSpan
    inline TextSpan(std::string_view text)
        : begin{text.begin()}
        , end{text.end()} {}

    operator std::string_view() const { return {begin, end}; }

    std::string_view::iterator begin, end;
};

using Text = std::string;

/// @brief constructs correctly indented text
/// builder with state accessors
class TextBuilder {
public:
    /// retrieve the build text
    auto build() const -> std::string_view { return m_buffer; }

    /// returns the current indentation
    auto indentation() const -> std::string_view { return m_indentation; }

    /// returns the current line
    auto line() const -> int { return m_line; }

    /// returns the current column
    auto column() const -> int;

    /// returns true, if no text is on the current line
    auto isBlankLine() const -> bool { return 0 == m_column; }

    /// change the indentation for the next line
    void setIndentation(std::string indent) { m_indentation = std::move(indent); }

    /// add part of a line
    auto operator<<(std::string_view) -> TextBuilder&;

    /// explicitly add a line break
    auto operator<<(NewLine) -> TextBuilder&;

private:
    std::string m_buffer;
    std::string m_indentation;
    int m_line = 1;
    int m_column = 0;
};

} // namespace Twofold::intern
