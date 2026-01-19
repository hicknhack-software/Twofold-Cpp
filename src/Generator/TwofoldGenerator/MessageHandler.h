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
#include "TwofoldGenerator/SourceMap.h"

#include <memory>

namespace Twofold {

class MessageHandler;
using MessageHandlerPtr = std::shared_ptr<MessageHandler>;

enum class MessageType { Info, Warning, Error };

/// @brief simple implementation of a message handler
class MessageHandler {
public:
    using Type = Twofold::MessageType;
    using Text = std::string;
    using Position = Twofold::FilePosition;

public:
    MessageHandler() = default;
    MessageHandler(MessageHandler const&) = delete;
    MessageHandler& operator=(MessageHandler const&) = delete;
    virtual ~MessageHandler() = default;

    virtual void message(Type, Text const&) = 0;
    virtual void templateMessage(Type, Position const&, Text const&) = 0;
};

} // namespace Twofold
