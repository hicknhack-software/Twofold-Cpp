// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

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

#include <SourceMap/RevisionThree.h>
#include <format>
#include <stdexcept>

namespace Twofold {
using namespace intern;

namespace {

struct BuilderPrivate {
    using Path = FileHandler::Path;
    std::filesystem::path const& outputBasePath;
    MessageHandlerPtr const& messageHandler;
    FileHandlerPtr const& fileHandler;
    Path sourceRoot;
    std::string sourceKey;
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

    void saveCode(Path const& input, Code const& code) {
        auto output = outputBasePath / outputFilePath(input);
        auto map = SourceMap::RevisionThree{};
        map.storeFile(output.filename());
        map.encodeMappings(code.sourceMap);
        auto mapName = output.filename().string() + ".map";
        if (fileHandler->save(Path{output.string() + ".map"}, map.toJson()) != FileHandler::SaveStatus::Success ||
            fileHandler->save(output, code.content + "\n//# sourceMappingURL=" + mapName + "\n") !=
                FileHandler::SaveStatus::Success)
            throw std::runtime_error("Could not save prepared template and map: " + output.string());
    }

    void buildInclude(Path const& file_path, std::string const& text) {
        auto oldCodeBuilder = std::exchange(codeBuilder, CodeBuilder{sourceRoot, sourceKey});

        lineProcessor(file_path, text);
        auto const code = codeBuilder.build();
        saveCode(file_path, code);

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
        saveCode(result.name, code);
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
        .sourceRoot = config.sourceRoot.empty() ? inputPath.parent_path() : config.sourceRoot,
        .sourceKey = config.sourceKey,
        .codeBuilder =
            CodeBuilder{config.sourceRoot.empty() ? inputPath.parent_path() : config.sourceRoot, config.sourceKey},
        .basePath = {},
    };
    builder.build(inputPath);
}

} // namespace Twofold
