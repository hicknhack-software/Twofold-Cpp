// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "TwofoldGenerator/MessageHandler.h"

#include <format>
#include <ostream>

namespace Twofold {

class MessageHandlerOstream : public MessageHandler {
public:
    explicit MessageHandlerOstream(std::ostream& output)
        : m_output(output) {}

    auto hasErrors() const -> bool { return m_hasErrors; }

    void message(Type type, Text const& text) override {
        if (type == Type::Error) m_hasErrors = true;
        switch (type) {
        case Type::Info: m_output << "Info:" << text << '\n'; break;
        case Type::Warning: m_output << "Warning:" << text << '\n'; break;
        case Type::Error: m_output << "Error:" << text << std::endl; break;
        default: m_output << "Unknown:" << text << '\n';
        }
    }

    void templateMessage(Type type, Position const& position, Text const& text) override {
        auto const composed =
            std::format("{}:{} Template Error: {}", position.name.string(), position.position.line, text);
        this->message(type, composed);
    }

private:
    std::ostream& m_output;
    bool m_hasErrors = false;
};

} // namespace Twofold
