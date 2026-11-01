// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "TwofoldGenerator/MessageHandler.h"

namespace Twofold::intern {

class CodeBuilder;
struct FileLine;

namespace Line {

/// @brief line processing to output text.
/// Supports inline expressions like #{ javascript-expression }
/// and escape sequences like ##{ javascript-expression }
/// in order to output '#{ javascript-expression }'.
class Interpolation {
public:
    Interpolation(MessageHandlerPtr messageHandler, CodeBuilder& builder)
        : m_messageHandler{std::move(messageHandler)}
        , m_builder{builder} {}

    void operator()(FileLine const& line) const;

protected:
    MessageHandlerPtr m_messageHandler;
    CodeBuilder& m_builder;
};

} // namespace Line
} // namespace Twofold::intern
