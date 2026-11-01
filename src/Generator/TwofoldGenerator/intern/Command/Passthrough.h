// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace Twofold::intern {

class CodeBuilder;
struct LineCommand;
enum class LineCommandResult;

namespace Command {

/// @brief This line processing could be used as default behavior.
/// It passthrough the input line to the output.
class Passthrough {
public:
    explicit Passthrough(CodeBuilder& builder)
        : m_builder{builder} {}

    auto operator()(LineCommand const&) const -> LineCommandResult;

private:
    CodeBuilder& m_builder;
};

} // namespace Command
} // namespace Twofold::intern
