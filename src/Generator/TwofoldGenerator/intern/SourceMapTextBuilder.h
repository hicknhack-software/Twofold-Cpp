// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "TwofoldGenerator/SourceMap.h"
#include "TwofoldGenerator/intern/LineProcessor.h"
#include "TwofoldGenerator/intern/TextBuilder.h"
#include <SourceMap/TextPosition.h>

namespace Twofold::intern {

/// @brief data for a originated text span
struct OriginText {
    inline OriginText(
        FilePosition origin, TextSpan span, Twofold::Interpolation interpolation = Twofold::Interpolation::OneToOne)
        : origin{std::move(origin)}
        , span{span}
        , interpolation{interpolation} {}

    /// construct from a FileLine of a LineProcessor
    inline OriginText(
        FileLine const& line, TextSpan span, Twofold::Interpolation interpolation = Twofold::Interpolation::OneToOne)
        : origin{
              .name = line.position.name,
              .position =
                  {line.position.position.line, 1 + SourceMap::utf16Length(std::string_view{line.begin, span.begin})}}
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

struct TargetText {
    std::string text;
};

struct SourceMapText {
    SourceMapping const sourceMap;
    std::string const text;
};

/// @brief Builds Text and SourceMap
/// makes sure both are consistent
class SourceMapTextBuilder {
public:
    SourceMapText build() const;

    inline auto isBlankLine() const -> bool { return m_textBuilder.isBlankLine(); }
    inline auto indentation() const -> std::string_view { return m_textBuilder.indentation(); }
    inline void setIndentation(std::string indent) { m_textBuilder.setIndentation(std::move(indent)); }

    void pushCaller(FilePosition const& originPosition);
    void popCaller();

    auto operator<<(OriginText const& originText) -> SourceMapTextBuilder&;
    auto operator<<(OriginNewLine const& originNewLine) -> SourceMapTextBuilder&;
    auto operator<<(TargetText const& targetText) -> SourceMapTextBuilder&;
    auto operator<<(NewLine const) -> SourceMapTextBuilder&;

private:
    TextBuilder m_textBuilder;
    SourceData m_sourceData;
    std::vector<CallerIndex> m_callerIndexStack;
};

} // namespace Twofold::intern
