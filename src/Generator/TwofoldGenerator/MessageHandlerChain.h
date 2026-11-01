// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "MessageHandler.h"

#include <memory>
#include <vector>

namespace Twofold {

class MessageHandlerChain; // local forward
using MessageHandlerChainPtr = std::shared_ptr<MessageHandlerChain>;

/// @brief MessageHandler implementation that allows multiple MessageHandlers
class MessageHandlerChain : public MessageHandler {
public:
    void add(MessageHandlerPtr);
    void remove(MessageHandlerPtr);

    // MessageHandler interface
public:
    void message(Type, Text const&) override;
    void templateMessage(Type, Position const&, Text const&) override;

private:
    std::vector<MessageHandlerPtr> m_handlers;
};

} // namespace Twofold
