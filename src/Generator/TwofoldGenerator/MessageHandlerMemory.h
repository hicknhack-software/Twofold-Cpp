// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "TwofoldGenerator/MessageHandler.h"

#include <span>
#include <vector>

namespace Twofold {

struct MemoryMessage {
    enum class Method { Message, TemplateMessage };

    Method method;
    MessageType type;
    FilePosition position;
    MessageHandler::Text text;
};

class MessageHandlerMemory : public MessageHandler {
public:
    void message(Type type, Text const& text) override {
        m_messages.push_back(
            MemoryMessage{
                MemoryMessage::Method::Message,
                type,
                {},
                text,
            });
    }

    void templateMessage(Type type, Position const& position, Text const& text) override {
        m_messages.push_back(
            MemoryMessage{
                MemoryMessage::Method::TemplateMessage,
                type,
                position,
                text,
            });
    }

    auto messages() const -> std::span<MemoryMessage const> { return m_messages; }

    void clear() { m_messages.clear(); }

private:
    std::vector<MemoryMessage> m_messages;
};

} // namespace Twofold
