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
#include "TwofoldRuntime/TargetBuilder.h"

namespace TwofoldRuntime {

using namespace intern;

void TargetBuilder::append(std::string_view text, FilePosition originPosition) {
    if (!text.empty()) {
        m_sourceMapBuilder << OriginText{
            std::move(originPosition), TextSpan{text.begin(), text.end()}, Interpolation::OneToOne};
    }
}

void TargetBuilder::appendExpression(std::string_view text, FilePosition originPosition) {
    if (!text.empty()) {
        m_sourceMapBuilder << OriginText{std::move(originPosition), text, Interpolation::None};
    }
}

void TargetBuilder::newLine(FilePosition originPosition) {
    m_sourceMapBuilder << OriginNewLine{std::move(originPosition)};
}

void TargetBuilder::pushIndentation(std::string_view indent, FilePosition originPosition) {
    auto fullIndent = std::string{indent};
    if (!m_indentationStack.empty()) fullIndent.insert(0, m_indentationStack.back().second);
    m_indentationStack.push_back(std::make_pair(std::string{indent}, fullIndent));
    m_sourceMapBuilder.pushCaller(std::move(originPosition));
    m_sourceMapBuilder.setIndentation(fullIndent);
}

void TargetBuilder::popIndentation() {
    m_indentationStack.pop_back();
    m_sourceMapBuilder.popCaller();

    auto newIndent = m_indentationStack.empty() ? std::string{} : m_indentationStack.back().second;
    m_sourceMapBuilder.setIndentation(newIndent);
}

void TargetBuilder::indentPart(std::string_view indent, FilePosition originPosition) {
    if (m_sourceMapBuilder.isBlankLine()) {
        m_partIndent = indent;
    }
    m_sourceMapBuilder << OriginText{std::move(originPosition), indent, Interpolation::OneToOne};
}

void TargetBuilder::pushPartIndent(FilePosition originPosition) { pushIndentation(m_partIndent, originPosition); }

void TargetBuilder::popPartIndent() {
    m_partIndent = m_indentationStack.back().first;
    popIndentation();
}

} // namespace TwofoldRuntime
