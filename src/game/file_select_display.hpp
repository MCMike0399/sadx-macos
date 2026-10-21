#pragma once
#include "engine/ui_scale_profile.hpp"
#include "engine/menu_draw_primitives.hpp"
#include "game/save_file_list.hpp"
#include "game/title_menu.hpp"

namespace sadx::file_select_display {
struct FrameRow {
    float right_x, bottom_y, depth;
    std::uint32_t top_color, bottom_color;
    std::uint8_t corner_texture, horizontal_texture, vertical_texture, padding;
};
struct SkyTile {
    std::int16_t texture, x, y;
};
struct Tables {
    std::array<FrameRow, 3> frame_rows;
    std::vector<SkyTile> sky_tiles;
    std::array<std::uint8_t, 4> detail_labels;
    std::array<std::uint8_t, 7> unlocked_icons, locked_icons, character_names;
    // Qualified source prefix for the game's stage IDs0..43;100 is tested first.
    std::array<std::uint8_t, 44> stage_names;
    std::array<std::uint16_t, 8> sequence_indices;
    std::array<std::uint32_t, 7> clear_flags;
    std::array<std::uint8_t, 10> digits;
};
enum class MenuTexture : std::uint32_t {
    Background = 0,
    Digits = 3,
    FileSelect = 7,
    FileCommon = 8,
    StageNames = 11,
    MemoryCard = 13,
    CharacterNames = 15,
    Emblem = 22,
    ScrollArrows = 24,
    MetalName = 28
};
struct Calendar {
    std::uint16_t year, month, day, hour, minute, second;
};
struct View {
    const float &horizontal_scale, &vertical_scale;
    const std::int32_t &display_disabled;
    const std::uint32_t &screen_width, &screen_height;
    save::Data &save;
    std::array<std::uint8_t, 512> &event_flags;
    const std::uint8_t &unlock_override, &tutorial_variant, &selection_has_files;
    const std::int16_t &first_visible_file;
    const std::uint8_t &file_count;
    std::uint8_t &selected_card;
    std::uint8_t &selected_file;
    const file_select::List &files;
    font::State &font;
    const Tables &tables;
    const std::uint8_t &first_message_visible, &second_message_visible;
    const TaskHandle &first_message, &second_message;
};
struct Calls : menu_draw::Calls {
    virtual void select_menu_texture(MenuTexture) = 0;
    // [platform:time] The source requests local calendar fields from real FILETIME.
    virtual Calendar local_calendar(std::uint64_t filetime) = 0;
    // Typed message task drawing remains an explicit native composition boundary.
    // STUB: SONIC 0x421920 MessageWindow_Display
    virtual void display_message(const TaskHandle &) = 0;
};
// FUNCTION: SONIC 0x505550 file_sel_disp
void draw(title_menu::FileSelectWork &, View &, pvr::State &, Calls &, X87Context &);
// NOTSA: selected native profile runs the real background scope, then draws
// foreground with that same owner after the source pop.
void draw_profiled(title_menu::FileSelectWork &, View &, pvr::State &, Calls &, X87Context &,
                    ui_scale::State &);
} // namespace sadx::file_select_display
