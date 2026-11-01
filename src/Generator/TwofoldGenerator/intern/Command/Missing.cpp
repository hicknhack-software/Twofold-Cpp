// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#include "Missing.h"

#include "TwofoldGenerator/intern/Line/Command.h"
#include "TwofoldGenerator/intern/LineProcessor.h"

#include <format>

namespace Twofold::intern::Command {

void Missing::operator()(LineCommand const& command) const {
    auto message = std::format("unknown command {}", std::string{command.begin, command.end});
    m_messageHandler->templateMessage(MessageType::Error, command.line.position, message);
}

} // namespace Twofold::intern::Command
