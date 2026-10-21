#pragma once
#include "game/save_data.hpp"
#include "game/title_menu.hpp"

namespace sadx::main_menu {
using Work = title_menu::MainMenuWork;
using Sequence = title_menu::SequenceWork;
using Mode = title_menu::MenuMode;
using Phase = title_menu::TitlePhase;

// NOTSA: borrows the existing title/session owners consumed by initialization.
struct InitState {
    std::int32_t &text_language, &voice_language;
    const std::uint8_t &tutorial_variant;
    std::uint8_t &title_count;
    title_menu::TitleWork &title;
    Work &canonical_menu;
};
struct InitCalls {
    virtual ~InitCalls() = default;
    virtual std::int8_t text_language() = 0;
    virtual std::int8_t voice_language() = 0;
    virtual void load_mode_textures(Mode) = 0;
    virtual void request_menu_music(std::int32_t) = 0;
};
// FUNCTION: SONIC 0x50bd20 title_menu_initialize
void initialize(Work &, InitState, InitCalls &);

// TitleMenuSbMdEnum.
enum class ControlStage : std::int32_t { Stay, Decide, Trial, ToDemo, DemoDelay, Reopen, Wait };
enum class Dialog : std::int32_t { Main = 2, MainWithoutPuti = 3, Trial = 4 };
// NOTSA: borrowed bootstrap/save/sequence owners; mapping includes the source cancel entry.
struct ControlState {
    const std::int32_t &ready;
    std::uint8_t &tutorial_variant, &trial_subgame;
    std::uint32_t &exit_requested;
    save::Data &save, &backup;
    Sequence &sequence;
    const std::array<std::uint8_t, 7> &variant_mapping;
};
struct ControlCalls {
    virtual ~ControlCalls() = default;
    virtual bool trial_enabled() = 0;
    virtual bool mission_enabled() = 0;
    // NOTSA: delegates the shared source eligibility predicate.
    virtual bool optional_entry_enabled() = 0;
    virtual void open_dialog(Dialog, std::int8_t cursor, const std::array<std::int8_t, 3> &disabled,
                             bool quick) = 0;
    virtual std::int8_t dialog_result() = 0;
    virtual bool trial_stages_enabled() = 0;
    virtual bool minigames_enabled() = 0;
    // NOTSA: delegates the recovered canonical-task navigation helper.
    virtual void prepare_character_select(std::int32_t) = 0;
    virtual void transition_menu(Mode) = 0;
    virtual std::int32_t music_state() = 0;
    virtual std::int16_t dialog_idle_frames() = 0;
    virtual void reset_dialog_idle_frames() = 0;
    virtual bool close_dialog() = 0;
    virtual void apply_save() = 0;
};
// FUNCTION: SONIC 0x50b630 title_menu_controls
void update_controls(Work &, ControlState, ControlCalls &);

struct UpdateCalls : InitCalls {
    virtual std::uint32_t fade_out_color(float) = 0;
    virtual std::uint32_t fade_in_color(float) = 0;
    virtual void update_controls(Work &) = 0;
    virtual void release_mode_textures(Mode) = 0;
};
// FUNCTION: SONIC 0x50bde0 title_menu_exec
void update(Work &, Sequence &, InitState, const std::uint32_t &logic_ticks, UpdateCalls &,
            X87Context &);

enum class TextureList { Common, MainMenu };
struct DrawCalls {
    virtual ~DrawCalls() = default;
    virtual void default_blending() = 0;
    virtual void set_base_color(std::uint32_t) = 0;
    virtual void set_material(std::uint32_t) = 0;
    virtual void select_texture(TextureList) = 0;
    virtual void draw_scaled(std::uint32_t, float x, float y, float depth, float scale_x,
                             float scale_y) = 0;
};
// NOTSA: borrowed display scales and the shared post-menu drawing flag.
struct DrawState {
    const float &horizontal_scale, &vertical_scale;
    std::uint32_t &window_after_menu;
    const std::uint32_t &screen_width, &screen_height;
};
// FUNCTION: SONIC 0x50ba90 title_menu_draw_background
void draw_background(float depth, DrawState, DrawCalls &, X87Context &);
// FUNCTION: SONIC 0x50bcb0 title_menu_display
void draw(const Work &, DrawState, DrawCalls &, X87Context &);
// NOTSA: pinned loader's centered background wrapper is qualified only at the source view.
void draw_profiled(const Work &, DrawState, DrawCalls &, X87Context &);
} // namespace sadx::main_menu
