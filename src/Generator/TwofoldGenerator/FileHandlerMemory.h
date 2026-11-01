// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

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
