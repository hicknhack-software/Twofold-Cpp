// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#include "Passthrough.h"

#include "TwofoldGenerator/intern/CodeBuilder.h"
#include "TwofoldGenerator/intern/Line/Command.h"

namespace Twofold::intern::Command {

auto Passthrough::operator()(LineCommand const& command) const -> LineCommandResult {
    m_builder << OriginScript{.text = {command.line, TextSpan{command.line.begin, command.line.end}}};
    return LineCommandResult::Success;
}

} // namespace Twofold::intern::Command
