#pragma once
#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace sadx::native {
// NOTSA: imported PVM declarations, separate from currently loaded texture handles.
struct CharacterTextureArchive {
    std::string archive;
    std::string resource_name;
    std::size_t texture_count;
};
using CharacterTextureAssets = std::array<std::vector<CharacterTextureArchive>, 9>;
} // namespace sadx::native
