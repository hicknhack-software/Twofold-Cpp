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
#include "Interpolation.h"

#include "TwofoldGenerator/intern/CodeBuilder.h"
#include "TwofoldGenerator/intern/LineProcessor.h"

#include "TwofoldGenerator/intern/Cpp/BraceCounter.h"

#include <algorithm>
#include <cctype>

namespace Twofold::intern::Line {
namespace {

using BraceCounter = Twofold::intern::Cpp::BraceCounter;

constexpr auto HASH = '#';
constexpr auto CURLY_OPEN = '{';

constexpr bool isHash(char chr) { return chr == HASH; }

void reportError(MessageHandlerPtr const& messageHandler, FileLine const& line, std::string const& message) {
    messageHandler->templateMessage(MessageType::Error, line.position, message);
}

} // namespace

void Interpolation::operator()(FileLine const& line) const {
    auto const beginIndent = line.firstNonSpace + 1;
    auto begin = std::find_if_not(beginIndent, line.end, &isspace);
    m_builder << IndentTargetPart{.text = {line, TextSpan{beginIndent, begin}}};

    auto end = begin;
    while (begin != line.end) {
        end = std::find_if(end, line.end, isHash);
        if (end == line.end) break; // reached line end
        auto expressionBegin = end + 1;
        if (expressionBegin == line.end) break; // reached line end
        switch (*expressionBegin) {
        case HASH:
            m_builder << OriginTarget{OriginText{line, TextSpan{begin, expressionBegin}}};
            begin = end = expressionBegin + 1;
            continue; // double ExprInitiator - one is skipped

        case CURLY_OPEN:
            m_builder << OriginTarget{OriginText{line, TextSpan{begin, end}}};
            expressionBegin++;
            auto expressionEnd = BraceCounter::findExpressionEnd(expressionBegin, line.end);
            if (expressionEnd == line.end) {
                reportError(m_messageHandler, line, "Missing close bracket!");
                begin = line.end;
                break; // terminate as invalid javascript
            }
            m_builder << OriginScriptExpression{OriginText{line, TextSpan{expressionBegin, expressionEnd}}};
            begin = end = expressionEnd + 1; // skip closing brackets
            continue; // expression evaluated
        }
        end = expressionBegin + 1;
    }
    m_builder << OriginTarget{OriginText{line, TextSpan{begin, line.end}}};
}

} // namespace Twofold::intern::Line
