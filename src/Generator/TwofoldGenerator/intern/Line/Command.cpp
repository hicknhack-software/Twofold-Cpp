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
#include "Command.h"

#include "TwofoldGenerator/intern/LineProcessor.h"

#include <algorithm>
#include <cctype>

namespace Twofold::intern::Line {
namespace {

Command::Function& findFunction(Command::Map& map, Command::Function& fallback, LineCommand const& command) {
    auto it = map.find(std::string_view{command.begin, command.end});
    if (it != map.end()) return it->second;
    return fallback;
}

} // namespace

void Command::operator()(FileLine const& line) {
    auto const begin = std::find_if_not(line.firstNonSpace + 1, line.end, &isspace);
    auto const end = std::find_if(begin, line.end, &isspace);
    auto command = LineCommand{
        .line = line,
        .begin = begin,
        .end = end,
    };
    auto result = findFunction(m_map, m_fallback, command)(command);
    if (result == LineCommandResult::Fallback) {
        m_fallback(command);
    }
}

} // namespace Twofold::intern::Line
