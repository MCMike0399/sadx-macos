#pragma once
#include "game/title_menu.hpp"
#include <array>
#include <vector>

namespace sadx::native {
// NOTSA: imported transition data; original function pointers never enter this representation.
struct TitleAssets {
    std::vector<title_menu::SequenceData> sequences;
    std::array<std::size_t, 3> entries{};
    std::vector<title_menu::MenuTextureId> initial_textures;
    title_menu::StageSelectionData selection;
    title_menu::ModeTextureLists mode_textures;
    title_menu::BackgroundLayout background_layout{};
};
} // namespace sadx::native
