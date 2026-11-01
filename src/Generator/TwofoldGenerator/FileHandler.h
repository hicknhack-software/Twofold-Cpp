// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace Twofold {

class FileHandler; // local forward
using FileHandlerPtr = std::shared_ptr<FileHandler>;

/// @brief Interface for resolving, loading and saving text files
/// You can fetch files from memory, network or file systems.
class FileHandler {
public:
    enum class LoadStatus {
        Success, ///< File was found and loading was sucessfull
        NotFound, ///< File was not found
        ErrorLoading, ///< File was found, error occured while loading it
    };
    enum class SaveStatus {
        Success, ///< File was found and loading was sucessfull
        ErrorSaving, ///< File could not be saved
    };
    using Text = std::string;
    using Path = std::filesystem::path;

    struct LoadResult {
        LoadStatus status;
        Path name; ///< resolved name (used in logs and sourceMaps) - only valid if found
        Text text; ///< content - empty unless success
    };

public:
    FileHandler() = default;
    FileHandler(FileHandler const&) = delete;
    FileHandler& operator=(FileHandler const&) = delete;
    virtual ~FileHandler() = default;

    virtual auto load(Path const&) const -> LoadResult = 0;
    virtual auto save(Path const&, std::string_view content) const -> SaveStatus = 0;
};

} // namespace Twofold
