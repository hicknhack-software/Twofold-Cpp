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
#include "SourceMapTextBuilder.h"

namespace Twofold::intern {

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
    if (originText.span.begin == originText.span.end) return *this; // empty span would create double entries

    auto callerIndex = m_callerIndexStack.empty() ? CallerIndex{} : m_callerIndexStack.back();

    auto& callers = SourceMap::get<ExtCaller>(m_sourceData);
    auto const indexValue = callerIndex.value;
    if (indexValue >= 0 && static_cast<CallerList::size_type>(indexValue) < callers.size()) {
        // this happens for expressions #{<expr>} because pushCaller and this operator is
        // called with the same originPosition
        if (callers[indexValue].original == originText.origin) {
            // take parent index
            callerIndex.value = callers[indexValue].parentIndex.value;

            // remove caller
            callers.erase(callers.begin() + indexValue);
        }
    }

    // m_sourceData.addEntry({{m_textBuilder.line(), m_textBuilder.column()},
    //                         originText.origin,
    //                         std::make_tuple(originText.interpolation, callerIndex)});

    m_textBuilder << originText.span;
    return *this;
}

auto SourceMapTextBuilder::operator<<(OriginNewLine const&) -> SourceMapTextBuilder& {
    // auto const callerIndex = m_callerIndexStack.empty() ? CallerIndex{} : m_callerIndexStack.back();
    // auto const column = isBlankLine() ? std::max(1, originNewLine.origin.column - 1) : originNewLine.origin.column;
    // m_sourceData.addEntry(
    //     {{m_textBuilder.line(), m_textBuilder.column()},
    //      {originNewLine.origin.name, {originNewLine.origin.line, column}},
    //      std::make_tuple(Interpolation::None, callerIndex)});

    m_textBuilder << NewLine();
    return *this;
}

auto SourceMapTextBuilder::operator<<(TargetText const& targetText) -> SourceMapTextBuilder& {
    m_textBuilder << targetText.text;
    return *this;
}

auto SourceMapTextBuilder::operator<<(NewLine const) -> SourceMapTextBuilder& {
    m_textBuilder << NewLine();
    return *this;
}

} // namespace Twofold::intern
