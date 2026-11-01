// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#include "TwofoldRuntime/TargetBuilder.h"

namespace TwofoldRuntime {

using namespace intern;

auto TargetBuilder::pushSource(FilePosition origin) -> bool {
    // Re-entering the same AST node does not introduce a new input context,
    // even when a template call frame lies between the two source scopes.
    if (!origin.isValid() || (!m_sourceOrigins.empty() && m_sourceOrigins.back() == origin)) return false;
    m_sourceMapBuilder.pushCaller(origin);
    m_sourceOrigins.push_back(std::move(origin));
    return true;
}

void TargetBuilder::popSource() {
    m_sourceMapBuilder.popCaller();
    m_sourceOrigins.pop_back();
}

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
