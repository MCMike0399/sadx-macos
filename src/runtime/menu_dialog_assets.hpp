#pragma once
#include "game/menu_dialog.hpp"
#include <string>

namespace sadx::native {
// NOTSA: pointer-free imported definitions; texture identities bind to the existing menu lists.
struct MenuDialogAssets {
    menu_dialog::Catalog definitions;
    std::vector<std::uint32_t> panel_texture_bindings;
    menu_dialog::Tables tables{};
    std::uint32_t cursor_texture_binding = 0;
    std::array<std::array<std::shared_ptr<const std::string>, 5>, 3> help{};
};
} // namespace sadx::native
