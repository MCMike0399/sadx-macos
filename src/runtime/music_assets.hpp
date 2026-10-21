#pragma once
#include "game/music.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace sadx::native {
// NOTSA: decoded imported audio is resource data, never original player addresses.
struct MusicClipAsset {
    std::int32_t song;
    std::string source_path;
    std::string pcm_path;
    std::uint64_t frames;
};
struct MusicAssets {
    music::Catalog catalog;
    std::vector<MusicClipAsset> clips;
};
} // namespace sadx::native
