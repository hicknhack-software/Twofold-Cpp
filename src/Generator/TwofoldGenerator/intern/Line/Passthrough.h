// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace Twofold::intern {

class CodeBuilder;
struct FileLine;

namespace Line {

/// @brief This line processing could be used as default behavior.
/// It passthrough the input line to the output.
class Passthrough {
public:
    explicit Passthrough(CodeBuilder& builder)
        : m_builder{builder} {}

    void operator()(FileLine const& line) const;

private:
    CodeBuilder& m_builder;
};

} // namespace Line
} // namespace Twofold::intern
