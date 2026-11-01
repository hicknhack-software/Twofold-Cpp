// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

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
