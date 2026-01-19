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
