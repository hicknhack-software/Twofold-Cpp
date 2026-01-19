#pragma once
#include "TwofoldGenerator/FileHandler.h"

namespace Twofold {

/// @brief Implementation of FileHandler using std::fstream
class FileHandlerSimple : public FileHandler {
public:
    auto load(Path const&) const -> LoadResult override;
    auto save(Path const&, std::string_view content) const -> SaveStatus override;
};

} // namespace Twofold
