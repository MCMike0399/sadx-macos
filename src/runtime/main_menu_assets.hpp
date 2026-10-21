#pragma once
#include "game/main_menu.hpp"
#include "game/main_menu_content.hpp"

namespace sadx::native {
// NOTSA: owns only imported source definitions2/3/4; selected textures borrow the menu pool.
struct MainMenuAssets {
    std::array<menu_dialog::Parameters, 3> definitions{};
    std::array<std::uint8_t, 7> variant_mapping{};
    main_menu_content::Layout trial_layout{};
    const menu_dialog::Parameters &definition(std::int32_t identity) const {
        if (identity < 2 || identity > 4)
            throw menu_dialog::Error(menu_dialog::Boundary::DialogIndex,
                                     "requested MainMenu dialog is not imported");
        return definitions[static_cast<std::size_t>(identity - 2)];
    }
};
} // namespace sadx::native
