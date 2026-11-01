// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "TwofoldGenerator/intern/LineProcessor.h"

#include <string_view>

namespace Twofold::intern::Cpp {

class BraceCounter {
public:
    using It = std::string_view::iterator;

    /// @return the nesting level of braces at the end
    /// >0 means that not all braces were closed
    /// <0 means more braces are closed than opened
    ///
    /// It does not enforce that the braces are matching pairs
    static auto countExpressionDepth(std::string_view text, int beginDepth = 0) -> int;

    /// @return the iterator to the end of the javascript expression
    /// will return end if no end of the expression was found
    static auto findExpressionEnd(It begin, It end) -> It;
};

} // namespace Twofold::intern::Cpp
