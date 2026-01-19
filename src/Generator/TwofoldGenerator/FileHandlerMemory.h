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

#include <map>
#include <span>
#include <string>
#include <vector>

namespace Twofold {

/// @brief helper for testing, stores files in memory
class FileHandlerMemory : public FileHandler {
public:
    struct SavedFile {
        Path savePath;
        std::string generatedContent;
    };
    using FileMap = std::map<Path, std::string>;

public:
    explicit FileHandlerMemory(FileMap files)
        : m_files(std::move(files)) {}

    auto load(Path const& file_path) const -> LoadResult override {
        auto it = m_files.find(file_path);
        if (it == m_files.end()) {
            return {LoadStatus::NotFound, {}, {}};
        }
        return {LoadStatus::Success, file_path, it->second};
    }

    auto save(Path const& savePath, std::string_view content) const -> SaveStatus override {
        m_savedFiles.push_back({savePath, std::string{content}});
        return SaveStatus::Success;
    }

    auto savedFiles() const -> std::span<SavedFile const> { return m_savedFiles; }

    void clearSavedFiles() { m_savedFiles.clear(); }

private:
    FileMap m_files;
    mutable std::vector<SavedFile> m_savedFiles;
};

} // namespace Twofold
