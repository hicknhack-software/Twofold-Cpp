// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#include "SourceMapTextBuilder.h"

namespace TwofoldRuntime::intern {

auto SourceMapTextBuilder::build() const -> SourceMapText {
    return SourceMapText{SourceMapping(m_sourceData), std::string{m_textBuilder.build()}};
}

void SourceMapTextBuilder::pushCaller(FilePosition const& originPosition) {
    auto parentIndex = m_callerIndexStack.empty() ? CallerIndex{} : m_callerIndexStack.back();
    auto index = CallerIndex{(int)SourceMap::get<ExtCaller>(m_sourceData).size()};
    SourceMap::get<ExtCaller>(m_sourceData).push_back(Caller{originPosition, parentIndex});
    m_callerIndexStack.push_back(index);
}

void SourceMapTextBuilder::popCaller() { m_callerIndexStack.pop_back(); }

auto SourceMapTextBuilder::operator<<(OriginText const& originText) -> SourceMapTextBuilder& {
    auto text = std::string_view{originText.span};
    auto origin = originText.origin;
    while (!text.empty()) {
        auto const newline = text.find('\n');
        auto const length = newline == text.npos ? text.size() : newline + 1;
        auto caller = m_callerIndexStack.empty() ? CallerIndex{} : m_callerIndexStack.back();
        auto const& callers = SourceMap::get<ExtCaller>(m_sourceData);
        // The expression itself already supplies this origin. Never erase a caller:
        // indices are shared by earlier entries and by active stack frames.
        if (caller.value >= 0 && callers[caller.value].original == origin) caller = callers[caller.value].parentIndex;
        if (m_textBuilder.isBlankLine())
            m_sourceData.addEntry(
                {.generated = {m_textBuilder.line(), 1}, .original = {}, .name = {}, .extensionData = {}});
        m_sourceData.addEntry({
            .generated = {m_textBuilder.line(), m_textBuilder.column()},
            .original = origin,
            .name = {},
            .extensionData = std::make_tuple(originText.interpolation, caller),
        });
        m_textBuilder << text.substr(0, length);
        // Stop a mapping at the end of its span (also prevents mapping the trailer).
        m_sourceData.addEntry(
            {.generated = {m_textBuilder.line(), m_textBuilder.column()},
             .original = {},
             .name = {},
             .extensionData = {}});
        if (originText.interpolation == Interpolation::OneToOne && newline != text.npos) {
            ++origin.position.line;
            origin.position.column = 1;
        }
        text.remove_prefix(length);
    }
    return *this;
}

auto SourceMapTextBuilder::operator<<(OriginNewLine const& newline) -> SourceMapTextBuilder& {
    auto const caller = m_callerIndexStack.empty() ? CallerIndex{} : m_callerIndexStack.back();
    m_sourceData.addEntry({
        .generated = {m_textBuilder.line(), m_textBuilder.column()},
        .original = newline.origin,
        .name = {},
        .extensionData = std::make_tuple(Interpolation::None, caller),
    });
    m_textBuilder << NewLine{};
    m_sourceData.addEntry({.generated = {m_textBuilder.line(), 1}, .original = {}, .name = {}, .extensionData = {}});
    return *this;
}

auto SourceMapTextBuilder::operator<<(NewLine const) -> SourceMapTextBuilder& {
    m_textBuilder << NewLine();
    return *this;
}

} // namespace TwofoldRuntime::intern
