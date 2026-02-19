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
