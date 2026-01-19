#include "FileHandlerSimple.h"

#include <filesystem>
#include <fstream>
#include <iterator>

namespace Twofold {

auto FileHandlerSimple::load(Path const& file_path) const -> LoadResult {
    if (!std::filesystem::exists(file_path)) {
        return {LoadStatus::NotFound, {}, {}};
    }

    auto ifs = std::ifstream{file_path};
    if (ifs.fail() && !ifs.eof()) {
        return {LoadStatus::ErrorLoading, {}, {}};
    }

    auto const content = std::string(std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>{});
    return {LoadStatus::Success, file_path, content};
}

auto FileHandlerSimple::save(Path const& filePath, std::string_view content) const -> SaveStatus {
    std::filesystem::create_directories(filePath.root_path());

    auto ofs = std::ofstream{filePath, std::ios::trunc};
    if (ofs.fail()) {
        return SaveStatus::ErrorSaving;
    }

    ofs << content;
    if (ofs.fail()) {
        return SaveStatus::ErrorSaving;
    }
    return SaveStatus::Success;
}

} // namespace Twofold
