#pragma once
#include "game/title_menu.hpp"

namespace sadx::character_select_menu {
using Work = title_menu::CharacterSelectWork;
using Mode = title_menu::MenuMode;
using Phase = title_menu::TitlePhase;
using Sequence = title_menu::SequenceWork;
// NOTSA: borrows the title sequence, input, and selection owners used by this menu.
struct State {
    const std::uint8_t &variant;
    std::uint8_t &selected_character, &character_cursor;
    const std::int32_t &ready, &logic_ticks;
    const std::uint32_t &pressed_buttons;
    std::int32_t &frame_increment, &saved_frame_increment;
    std::uint16_t &player_character;
    const std::uint8_t &trial_subgame;
    Sequence &sequence;
};
struct Calls {
    virtual ~Calls() = default;
    virtual void load_mode_textures(Mode) = 0;
    virtual void create_preview() = 0;
    virtual void request_music(std::int32_t) = 0;
    virtual void request_voice(std::int32_t) = 0;
    virtual bool character_enabled(std::int32_t) = 0;
    virtual bool trial_enabled(std::int32_t) = 0;
    virtual bool character_cleared(std::int32_t) = 0;
    virtual void release_loading_font() = 0;
    virtual void calculate_ticks() = 0;
    virtual void navigation_sound() = 0;
    virtual bool confirm_pressed() = 0;
    virtual bool cancel_pressed() = 0;
    virtual bool selection_enabled() = 0;
    virtual void transition_menu(Mode) = 0;
    virtual void cancel_sound() = 0;
    virtual void open_dialog(std::int32_t) = 0;
    virtual std::int8_t dialog_result() = 0;
    virtual void character_voice(std::int32_t) = 0;
    virtual void select_preview(std::int32_t) = 0;
    virtual std::int32_t character_count() = 0;
    virtual void set_stage(std::int16_t, std::int16_t) = 0;
    virtual void delete_preview() = 0;
    virtual std::uint32_t fade_in_color(float) = 0;
    virtual std::uint32_t fade_out_color(float) = 0;
    virtual void release_mode_textures(Mode) = 0;
};
// FUNCTION: SONIC 0x5114e0 FUN_005114e0
void initialize(Work &, State, Calls &);
// FUNCTION: SONIC 0x511570 FUN_00511570
void update_controls(Work &, State, Calls &, X87Context &);
// FUNCTION: SONIC 0x5122d0 CharacterSelectMenu_Main
void update(Work &, State, Calls &, X87Context &);
} // namespace sadx::character_select_menu

namespace sadx { class NinjaMath; }
namespace sadx::character_select_menu {
// NOTSA: imported source character-name geometry and borrowed screen scales.
struct DrawState {
    const float &horizontal_scale, &vertical_scale;
    const std::array<float, 7> &name_widths;
};
enum class TextureList { CharacterSelect, Digits };
struct DrawCalls {
    virtual ~DrawCalls() = default;
    virtual void default_blending() = 0;
    virtual void base_color(std::uint32_t) = 0;
    virtual void sky_background(float) = 0;
    virtual void select_texture(TextureList) = 0;
    virtual void material(std::uint32_t) = 0;
    virtual void colors(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) = 0;
    virtual void draw_texture(std::uint32_t, float, float, float) = 0;
    virtual void draw_scaled(std::uint32_t, float, float, float, float, float) = 0;
    virtual void title_back(float, float, float, float, float) = 0;
    virtual void set_depth_comparison(std::int32_t) = 0;
    virtual void restore_depth_comparison() = 0;
    virtual void round_panel(std::uint32_t, float, float, float, float, float) = 0;
    virtual std::uint32_t interpolate_colors(std::uint32_t, std::uint32_t, float) = 0;
    virtual void character_name(std::int32_t, std::uint32_t, float, float, float, float, float) = 0;
    virtual void character_label(std::int32_t, std::uint32_t, float, float, float) = 0;
    virtual std::uint32_t character_completion(std::int32_t) = 0;
    virtual void draw_number(std::uint32_t, std::int32_t, bool,
                             float, float, float, float) = 0;
};
// FUNCTION: SONIC 0x5119f0 FUN_005119f0
void draw_background(const Work &, State, DrawState, DrawCalls &, X87Context &);
// FUNCTION: SONIC 0x511e10 FUN_00511e10
void draw_labels(const Work &, State, Calls &, DrawCalls &, X87Context &);
// FUNCTION: SONIC 0x511e90 FUN_00511e90
void draw_carousel(const Work &, State, DrawState, Calls &, DrawCalls &, const NinjaMath &, X87Context &);
// FUNCTION: SONIC 0x512110 FUN_00512110
void draw_emblems(const Work &, State, DrawState, DrawCalls &, X87Context &);
} // namespace sadx::character_select_menu

namespace sadx::character_select_menu {
// FUNCTION: SONIC 0x512450 CharacterSelectMenu_Display
void draw(const Work &, State, DrawState, Calls &, DrawCalls &, const NinjaMath &, X87Context &);
// FUNCTION: SONIC 0x5114a0 FUN_005114a0
bool selection_enabled(std::int32_t, State, Calls &);
} // namespace sadx::character_select_menu

#include "game/main_menu_eligibility.hpp"
namespace sadx::character_select_menu {
// NOTSA: imported clear-flag records and live save completion share their existing owners.
struct Availability {
    menu_eligibility::Progress progress;
    menu_eligibility::Completion completion;
    const std::uint8_t &trial_subgame;
    const std::array<std::uint32_t, 7> &clear_flags;
};
// FUNCTION: SONIC 0x512c50 CountUnlockedCharacters
std::int32_t character_count(menu_eligibility::Progress);
// FUNCTION: SONIC 0x5063e0 IsAdventureComplete
bool character_cleared(std::uint32_t, Availability);
// FUNCTION: SONIC 0x506700 GetEachCharEnableForTrial
bool trial_character_enabled(std::uint32_t, Availability);
} // namespace sadx::character_select_menu
