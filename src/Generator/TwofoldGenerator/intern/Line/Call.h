// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "TwofoldGenerator/MessageHandler.h"

namespace Twofold::intern {

struct FileLine;
class CodeBuilder;

namespace Line {

/// @brief line processing for calling a javascript function
/// whose output is handled indentation aware.
class Call {
public:
    Call(MessageHandlerPtr messageHandler, CodeBuilder& builder)
        : m_messageHandler{messageHandler}
        , m_builder{builder} {}

    void operator()(FileLine const& line) const;

private:
    MessageHandlerPtr const m_messageHandler;
    CodeBuilder& m_builder;
};

} // namespace Line
} // namespace Twofold::intern
