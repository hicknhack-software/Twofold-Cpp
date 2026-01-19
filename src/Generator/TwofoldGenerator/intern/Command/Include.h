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
