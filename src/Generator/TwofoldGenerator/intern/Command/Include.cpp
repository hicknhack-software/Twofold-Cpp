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
#include "Include.h"

#include "TwofoldGenerator/intern/CodeBuilder.h"
#include "TwofoldGenerator/intern/Line/Command.h"

#include <algorithm>
#include <cassert>
#include <format>
#include <ranges>

namespace Twofold::intern::Command {
namespace {

static constexpr auto max_include_stack_depth = 500;

constexpr bool isDoubleQuote(char chr) { return chr == '"'; }

auto extractNameArgument(LineCommand const& command) -> TextSpan {
    using namespace std::string_view_literals;
    auto nameBegin = std::find_if(command.end, command.line.end, &isDoubleQuote);
    if (nameBegin == command.line.end) return ""sv; // no initial double quote found
    nameBegin++;
    auto nameEnd = std::find_if(nameBegin, command.line.end, &isDoubleQuote);
    if (nameEnd == command.line.end) return ""sv; // no second double quote found
    return TextSpan{nameBegin, nameEnd};
}

void reportParameterError(MessageHandlerPtr const& messageHandler, LineCommand const& command, char const* message) {
    messageHandler->templateMessage(MessageType::Error, command.line.position, message);
}

void reportLoaderError(
    MessageHandlerPtr const& messageHandler,
    LineCommand const& command,
    std::filesystem::path const& name,
    FileHandler::LoadStatus status) {
    auto const nameString = name.string();
    auto const message = (status == FileHandler::LoadStatus::NotFound) ? std::format("Could not find {}", nameString)
                                                                       : std::format("Could not load {}", nameString);
    messageHandler->templateMessage(MessageType::Error, command.line.position, message);
}

void reportStackError(
    MessageHandlerPtr const& messageHandler, LineCommand const& command, Include::Stack const& stack) {
    auto messageLines = std::vector<std::string>{};
    messageLines.push_back(std::format("Include depth too deep: {}", stack.size()));
    for (auto const& name : stack) {
        messageLines.push_back(std::format(" + included: {}", name.string()));
    }
    messageHandler->templateMessage(
        MessageType::Error,
        command.line.position,
        messageLines | std::views::join_with('\n') | std::ranges::to<std::string>());
}

struct StackEntry {
    StackEntry(Include::Stack& stack, std::filesystem::path name)
        : m_stack(stack)
#ifdef assert
        , m_name(name)
        , m_size(m_stack.size())
#endif
    {
        m_stack.push_back(std::move(name));
    }

    ~StackEntry() {
#ifdef assert
        assert(m_stack.size() == m_size + 1);
        assert(m_stack.back() == m_name);
#endif
        m_stack.pop_back();
    }

private:
    Include::Stack& m_stack;
#ifdef assert
    std::filesystem::path m_name;
    size_t m_size;
#endif
};

} // namespace

auto Include::operator()(LineCommand const& command) -> LineCommandResult {
    using namespace std::string_view_literals;
    auto relativePathSpan = extractNameArgument(command);
    auto relativePath = std::filesystem::path{std::string{relativePathSpan.begin, relativePathSpan.end}};
    static constexpr auto twofoldExtension = ".twofold"sv;
    if (!relativePath.filename().string().ends_with(twofoldExtension)) {
        return LineCommandResult::Fallback; // keep include intact
    }
    if (relativePath.empty()) {
        reportParameterError(m_messageHandler, command, "filename argument in \"quotes\" required!");
        return LineCommandResult::Error; // skip include continue with next line
    }
    auto const loadPath = std::filesystem::path{command.line.position.name}.replace_filename(relativePath);

    auto const loadResult = m_fileHandler->load(loadPath);
    if (loadResult.status != FileHandler::LoadStatus::Success) {
        reportLoaderError(m_messageHandler, command, loadPath, loadResult.status);
        return LineCommandResult::Error; // skip include continue with next line
    }

    auto stackEntry = StackEntry{m_stack, loadPath};
    (void)stackEntry;
    if (m_stack.size() > max_include_stack_depth) {
        reportStackError(m_messageHandler, command, m_stack);
        return LineCommandResult::Error; // skip include continue with next line
    }

    m_processIncludeText(loadResult.name, loadResult.text);
    m_builder << OriginPart{
        .text = {command.line, TextSpan{command.line.begin, relativePathSpan.end - twofoldExtension.size()}}};
    m_builder << OriginScript{.text = {command.line, TextSpan{relativePathSpan.end, command.line.end}}}; // final "
    return LineCommandResult::Success;
}

} // namespace Twofold::intern::Command
