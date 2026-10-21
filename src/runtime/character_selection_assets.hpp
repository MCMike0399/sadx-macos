#pragma once
#include "engine/character_pack.hpp"
#include "game/menu_dialog.hpp"
#include "math/ninja_math.hpp"
#include <array>
#include <vector>

namespace sadx::native {
// NOTSA: imported declaration metadata borrows handles from the single character pack.
struct SelectionActionStep {
    std::array<std::uint32_t, 3> source_key{};
    ActionHandle action;
    std::int32_t link_frames = 0;
};
struct SelectionActionSequence {
    std::uint16_t character = 0;
    std::vector<SelectionActionStep> steps;
};
struct CharacterSelectionAssets {
    std::array<std::uint16_t, 7> characters{};
    std::array<NinjaVector, 7> positions{};
    std::int32_t sonic_aim_y = 0;
    character_pack::ResourceId question_object = 0;
    std::uint32_t question_menu_texture = 0;
    std::array<SelectionActionSequence, 6> sequences{};
    std::array<float, 7> name_widths{};
    std::array<std::uint32_t, 7> clear_flags{};
    std::array<std::int32_t, 8> selected_voice_cues{};
    std::array<menu_dialog::Parameters, 4> definitions{};
    std::uint32_t dialog_menu_texture = 0;
    const menu_dialog::Parameters &definition(std::int32_t identity) const {
        if (identity < 7 || identity > 10)
            throw menu_dialog::Error(menu_dialog::Boundary::DialogIndex,
                                     "requested CharacterSelect dialog is not imported");
        return definitions[static_cast<std::size_t>(identity - 7)];
    }
};
} // namespace sadx::native
