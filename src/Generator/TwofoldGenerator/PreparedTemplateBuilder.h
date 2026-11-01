// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "TwofoldGenerator/FileHandler.h"
#include "TwofoldGenerator/MessageHandler.h"

namespace Twofold {

/// @brief Use this interface to process input files
struct PreparedTemplateBuilder {
    using Path = std::filesystem::path;
    struct Config {
        Path outputBasePath;
        MessageHandlerPtr messageHandler;
        FileHandlerPtr fileHandler;
        Path sourceRoot{};
        std::string sourceKey = "template";
    };
    PreparedTemplateBuilder(Config config)
        : config{std::move(config)} {}

    void build(Path const& inputFilePath);

private:
    Config config;
};

} // namespace Twofold
