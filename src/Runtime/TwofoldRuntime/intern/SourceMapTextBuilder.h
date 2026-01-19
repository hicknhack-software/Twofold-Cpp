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
#include "TwofoldRuntime/SourceMap.h"
#include "TwofoldRuntime/intern/TextBuilder.h"

namespace TwofoldRuntime::intern {

/// @brief data for a originated text span
struct OriginText {
    inline OriginText(
        FilePosition origin,
        TextSpan span,
        TwofoldRuntime::Interpolation interpolation = TwofoldRuntime::Interpolation::OneToOne)
        : origin{std::move(origin)}
        , span{span}
        , interpolation{interpolation} {}

    FilePosition origin; // position this entry is from
    TextSpan span; // text added to output
    Interpolation interpolation; // text generated or from source
};

struct OriginNewLine {
    inline OriginNewLine(FilePosition const& origin)
        : origin(origin) {}

    FilePosition origin; // position this entry is from
};

struct SourceMapText {
    SourceMapping const sourceMap;
    std::string const text;
};

/// @brief Builds Text and SourceMap
/// makes sure both are consistent
class SourceMapTextBuilder {
public:
    auto build() const -> SourceMapText;

    inline auto isBlankLine() const -> bool { return m_textBuilder.isBlankLine(); }
    inline auto indentation() const -> std::string_view { return m_textBuilder.indentation(); }
    inline void setIndentation(std::string indent) { m_textBuilder.setIndentation(std::move(indent)); }

    void pushCaller(FilePosition const& originPosition);
    void popCaller();

    auto operator<<(OriginText const& originText) -> SourceMapTextBuilder&;
    auto operator<<(OriginNewLine const& originNewLine) -> SourceMapTextBuilder&;
    auto operator<<(NewLine const) -> SourceMapTextBuilder&;

private:
    TextBuilder m_textBuilder;
    SourceData m_sourceData;
    std::vector<CallerIndex> m_callerIndexStack;
};

} // namespace TwofoldRuntime::intern
