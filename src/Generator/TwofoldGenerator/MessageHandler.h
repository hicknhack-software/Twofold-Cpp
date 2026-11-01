// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

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
