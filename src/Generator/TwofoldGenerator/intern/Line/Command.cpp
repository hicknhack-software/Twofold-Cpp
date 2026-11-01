// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

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
