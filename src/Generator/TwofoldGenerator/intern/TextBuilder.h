// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

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
