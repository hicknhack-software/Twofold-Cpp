// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "TwofoldGenerator/FileHandler.h"
#include "TwofoldGenerator/MessageHandler.h"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace Twofold::intern {

class CodeBuilder;
struct LineCommand;
enum class LineCommandResult;

namespace Command {

/// @brief This command includes a template
/// and returns the rendered content of that file.
/// Included templates have access to the variables of
/// the active context. If you are using the PathTextFileLoader,
/// the templates are looked for in the paths defined by it.
class Include {
public:
    using ProcessIncludedTextFunction =
        std::move_only_function<void(std::filesystem::path const& name, std::string const& text)>;
    using Stack = std::vector<std::filesystem::path>;

    Include(
        MessageHandlerPtr messageHandler,
        FileHandlerPtr fileHandler,
        CodeBuilder& builder,
        ProcessIncludedTextFunction processIncludeText)
        : m_messageHandler{std::move(messageHandler)}
        , m_fileHandler{std::move(fileHandler)}
        , m_builder{builder}
        , m_processIncludeText{std::move(processIncludeText)} {}

    auto operator()(LineCommand const& command) -> LineCommandResult;

private:
    MessageHandlerPtr m_messageHandler;
    FileHandlerPtr m_fileHandler;
    CodeBuilder& m_builder;
    ProcessIncludedTextFunction m_processIncludeText;
    Stack m_stack;
};

} // namespace Command
} // namespace Twofold::intern
