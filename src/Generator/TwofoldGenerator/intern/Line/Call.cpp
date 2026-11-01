// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#include "Call.h"

#include "TwofoldGenerator/intern/CodeBuilder.h"
#include "TwofoldGenerator/intern/Cpp/BraceCounter.h"
#include "TwofoldGenerator/intern/LineProcessor.h"

#include <cctype>

namespace Twofold::intern::Line {

using BraceCounter = Twofold::intern::Cpp::BraceCounter;

void Call::operator()(FileLine const& line) const {
    using namespace std::string_literals;
    auto begin = line.firstNonSpace + 1;
    auto end = std::find_if_not(begin, line.end, &isspace);

    auto depth = BraceCounter::countExpressionDepth(std::string_view{end, line.end});
    if (0 < depth) {
        m_messageHandler->templateMessage(MessageType::Error, line.position, "expression is not terminated"s);
    }
    if (0 > depth) {
        m_messageHandler->templateMessage(MessageType::Error, line.position, "expression is invalid"s);
    }

    if (0 == depth) m_builder << PushTargetIndentation{OriginText{line, TextSpan{begin, end}}};
    m_builder << OriginScriptCall{OriginText{line, TextSpan{end, line.end}}};
    if (0 == depth) m_builder << PopTargetIndentation{OriginText{line, TextSpan{line.end, line.end}}};
}

} // namespace Twofold::intern::Line
