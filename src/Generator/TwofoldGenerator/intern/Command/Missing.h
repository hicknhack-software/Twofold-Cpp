// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "TwofoldGenerator/MessageHandler.h"

namespace Twofold::intern {

struct LineCommand;

namespace Command {

/// @brief This command could be used as default behavior
/// in order to compensate missing commands or user spelling
/// mistakes. It emits a error message to the handler.
class Missing {
public:
    Missing(MessageHandlerPtr messageHandler)
        : m_messageHandler{std::move(messageHandler)} {}

    void operator()(LineCommand const&) const;

private:
    MessageHandlerPtr const m_messageHandler;
};

} // namespace Command
} // namespace Twofold::intern
