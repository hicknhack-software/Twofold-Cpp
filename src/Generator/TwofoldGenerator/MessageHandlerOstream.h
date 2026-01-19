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
#pragma once
#include "TwofoldGenerator/MessageHandler.h"

#include <format>
#include <ostream>

namespace Twofold {

class MessageHandlerOstream : public MessageHandler {
public:
    explicit MessageHandlerOstream(std::ostream& output)
        : m_output(output) {}

    void message(Type type, Text const& text) override {
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
};

} // namespace Twofold
