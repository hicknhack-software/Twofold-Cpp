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
