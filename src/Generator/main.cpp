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
#include "TwofoldGenerator/FileHandlerPreview.h"
#include "TwofoldGenerator/FileHandlerSimple.h"
#include "TwofoldGenerator/MessageHandlerOstream.h"
#include "TwofoldGenerator/PreparedTemplateBuilder.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

struct Arguments {
    std::filesystem::path input_file_name;
    std::filesystem::path output_directory;
    bool is_preview = false;
};

void printUsage(std::string_view programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  -i, --input <file>     Input file\n"
              << "  -o, --output <dir>     Output directory\n"
              << "  --preview              Enable preview mode\n"
              << "  -h, --help             Print this help and exit\n";
}

Arguments parseArguments(int argc, char const** argv) {
    Arguments args;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            std::exit(0);
        }
        else if (arg == "-i" || arg == "--input") {
            if (i + 1 < argc) {
                args.input_file_name = argv[++i];
            }
            else {
                std::cerr << "Error: Missing input file.\n";
                std::exit(1);
            }
        }
        else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                args.output_directory = argv[++i];
            }
            else {
                std::cerr << "Error: Missing output directory.\n";
                std::exit(1);
            }
        }
        else if (arg == "--preview") {
            args.is_preview = true;
        }
        else {
            std::cerr << "Error: Unknown argument '" << arg << "'\n";
            std::cerr << "Use -h or --help for usage.\n";
            std::exit(1);
        }
    }
    if (args.input_file_name.empty()) {
        std::cerr << "Error: Missing input file.\n";
        std::cerr << "Use -h or --help for usage.\n";
        std::exit(1);
    }
    return args;
}

int main(int argc, char const** argv) {
    auto args = parseArguments(argc, argv);

    auto messageHandler = std::make_shared<Twofold::MessageHandlerOstream>(std::cerr);
    auto fileHandler = Twofold::FileHandlerPtr{std::make_shared<Twofold::FileHandlerSimple>()};

    auto previewHandler = std::shared_ptr<Twofold::FileHandlerPreview>{};
    if (args.is_preview) {
        fileHandler = (previewHandler = std::make_shared<Twofold::FileHandlerPreview>(fileHandler));
    }

    auto builder = Twofold::PreparedTemplateBuilder{{
        .outputBasePath = args.output_directory,
        .messageHandler = messageHandler,
        .fileHandler = fileHandler,
    }};
    builder.build(args.input_file_name);

    if (previewHandler) {
        for (auto const& file : previewHandler->savedFiles()) {
            std::cout << "  " << file.string() << "\n";
        }
    }

    return 0;
}
