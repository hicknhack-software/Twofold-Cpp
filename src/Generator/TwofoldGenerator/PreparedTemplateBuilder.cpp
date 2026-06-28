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
#include "PreparedTemplateBuilder.h"

#include "TwofoldGenerator/intern/Command/Include.h"
#include "TwofoldGenerator/intern/Command/Missing.h"
#include "TwofoldGenerator/intern/Command/Passthrough.h"

#include "TwofoldGenerator/intern/Line/Call.h"
#include "TwofoldGenerator/intern/Line/Command.h"
#include "TwofoldGenerator/intern/Line/InterpolateLine.h"
#include "TwofoldGenerator/intern/Line/Interpolation.h"
#include "TwofoldGenerator/intern/Line/Passthrough.h"

#include "TwofoldGenerator/intern/CodeBuilder.h"
#include "TwofoldGenerator/intern/LineProcessor.h"

#include <format>

namespace Twofold {
using namespace intern;

namespace {

struct BuilderPrivate {
    using Path = FileHandler::Path;
    std::filesystem::path const& outputBasePath;
    MessageHandlerPtr const& messageHandler;
    FileHandlerPtr const& fileHandler;
    CodeBuilder codeBuilder;
    LineProcessor lineProcessor{buildLineProcessor()};
    std::filesystem::path basePath;

    auto buildLineProcessor() -> LineProcessor {
        auto map = LineProcessor::Map{};
        map['\\'] = Line::Interpolation{messageHandler, codeBuilder};
        map['|'] = Line::InterpolateLine{messageHandler, codeBuilder};
        map['='] = Line::Call{messageHandler, codeBuilder};
        map['#'] = Line::Command{buildLineCommand()};
        return {std::move(map), Line::Passthrough{codeBuilder}};
    }

    auto buildLineCommand() -> Line::Command {
        using namespace std::string_view_literals;
        auto map = Line::Command::Map{};
        map["include"sv] = Command::Include{
            messageHandler, fileHandler, codeBuilder, [this](Path const& file_path, std::string const& text) {
                return buildInclude(file_path, text);
            }};
        return {std::move(map), Command::Passthrough{codeBuilder}};
    }

    void buildInclude(Path const& file_path, std::string const& text) {
        auto oldCodeBuilder = std::exchange(codeBuilder, CodeBuilder{});

        lineProcessor(file_path, text);
        auto const code = codeBuilder.build();
        fileHandler->save(outputBasePath / outputFilePath(file_path), code.content);

        std::exchange(codeBuilder, std::move(oldCodeBuilder));
    }

    void build(Path const& file_path) {
        basePath = file_path;
        basePath.remove_filename();
        auto const result = fileHandler->load(file_path);
        if (result.status != FileHandler::LoadStatus::Success) {
            messageHandler->message(MessageType::Error, std::format("Could not load {}", file_path.string()));
            return; // could not load
        }

        lineProcessor(result.name, result.text);
        auto const code = codeBuilder.build();
        fileHandler->save(outputBasePath / outputFilePath(result.name), code.content);
    }

    auto outputFilePath(std::filesystem::path inputPath) const -> std::filesystem::path {
        auto relative = std::filesystem::relative(inputPath, basePath);
        relative.replace_filename(relative.stem());
        return relative;
    }
};

} // namespace

void PreparedTemplateBuilder::build(FileHandler::Path const& inputPath) {
    auto builder = BuilderPrivate{
        .outputBasePath = config.outputBasePath,
        .messageHandler = config.messageHandler,
        .fileHandler = config.fileHandler,
        .codeBuilder = {},
        .basePath = {},
    };
    builder.build(inputPath);
}

} // namespace Twofold
