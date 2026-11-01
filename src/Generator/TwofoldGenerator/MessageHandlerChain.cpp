// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#include "MessageHandlerChain.h"

namespace Twofold {

void MessageHandlerChain::add(MessageHandlerPtr handler) { m_handlers.push_back(handler); }

void MessageHandlerChain::remove(MessageHandlerPtr handler) {
    m_handlers.erase(std::remove(m_handlers.begin(), m_handlers.end(), handler), m_handlers.end());
}

void MessageHandlerChain::message(Type type, Text const& text) {
    for (MessageHandlerPtr handler : m_handlers) {
        handler->message(type, text);
    }
}

void MessageHandlerChain::templateMessage(Type type, Position const& position, Text const& text) {
    for (MessageHandlerPtr handler : m_handlers) {
        handler->templateMessage(type, position, text);
    }
}

} // namespace Twofold
