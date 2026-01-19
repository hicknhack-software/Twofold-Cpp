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

#include <span>
#include <string>
#include <vector>

namespace Twofold {

class FileHandlerPreview : public FileHandler {
public:
    explicit FileHandlerPreview(FileHandlerPtr fileHandler)
        : m_fileHandler(std::move(fileHandler)) {}

    auto load(Path const& file_path) const -> LoadResult override { return m_fileHandler->load(file_path); }

    auto save(Path const& source_file_path, std::string_view /*content*/) const -> SaveStatus override {
        m_savedFiles.push_back(source_file_path);
        return SaveStatus::Success;
    }

    auto savedFiles() const -> std::span<Path const> { return m_savedFiles; }

private:
    FileHandlerPtr m_fileHandler;
    mutable std::vector<Path> m_savedFiles;
};

} // namespace Twofold
