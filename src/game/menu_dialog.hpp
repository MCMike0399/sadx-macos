#pragma once
#include "game/title_menu.hpp"
#include "math/ninja_math.hpp"
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace sadx::menu_dialog {
using Parameters = title_menu::DialogParameters;
using Work = title_menu::DialogWork;
using Catalog = std::vector<Parameters>;
// NOTSA: borrows the canonical global task; every work lookup uses that task's auxiliary owner.
struct Owner {
    const TaskHandle &task;
};
// NOTSA: bounded native resource view for the source negative-terminated byte list.
struct Choices {
    const std::int8_t *data = nullptr;
    std::size_t size = 0;
};
enum class Boundary {
    MissingTask,
    MissingWork,
    DisabledCapacity,
    DisabledTerminator,
    DialogIndex,
    MissingParameters,
    AnimationIndex,
    AnimationCounter,
    NavigationStyle
};
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, std::string message)
        : std::runtime_error(std::move(message)), boundary_(boundary) {}
    Boundary boundary() const { return boundary_; }

  private:
    Boundary boundary_;
};
struct Calls {
    virtual ~Calls() = default;
    // NOTSA: source dsPlay_oneshot(cue,0,0,0), bound to the existing native sound queue.
    virtual void play_sound(std::int32_t cue) = 0;
};
// FUNCTION: SONIC 0x4325e0 SetDialogCursor
void set_cursor(std::int8_t, Owner);
// FUNCTION: SONIC 0x432d20 OpenDialogCsrLet
void open_with_cursor(const Parameters &, std::int8_t, Choices disabled, Owner, Calls &);
// FUNCTION: SONIC 0x432db0 OpenDialog
void open(const Parameters &, Owner, Calls &);
// FUNCTION: SONIC 0x5057b0 MenuDialog_Req
void request(std::int32_t dialog, const Catalog &, Owner, Calls &);
// FUNCTION: SONIC 0x432550 GetDialogStat
std::int32_t result(Owner);
struct AnimationScale {
    float x, y;
};
struct DirectionMasks {
    std::uint32_t previous, next;
};
struct Tables {
    std::array<AnimationScale, 16> opening, closing;
    std::array<DirectionMasks, 6> directions;
};
struct UpdateCalls : Calls {
    virtual std::uint32_t pressed_buttons() = 0;
    // STUB: SONIC 0x42c4e0 ScreenFade_SetColor
    virtual void clear_screen_fade() = 0;
    virtual void set_back_colors(std::uint32_t, std::uint32_t, std::uint32_t) = 0;
    virtual void set_material(std::uint32_t) = 0;
    virtual void set_base_color(std::uint32_t) = 0;
    virtual void reset_material() = 0;
    virtual void default_blending() = 0;
};
// FUNCTION: SONIC 0x431e10 IsDialogChoiceDisabled
bool is_disabled(std::int8_t, const Work &, Owner);
// FUNCTION: SONIC 0x431e50 NextDialogChoice
std::int8_t next_choice(std::int8_t, std::int8_t count, const Work &, Owner);
// FUNCTION: SONIC 0x431eb0 PreviousDialogChoice
std::int8_t previous_choice(std::int8_t, const Work &, Owner);
// FUNCTION: SONIC 0x432720 MenuObj_Main
void update(Work &, Owner, const Tables &, const std::uint32_t &logic_ticks, UpdateCalls &,
            const NinjaMath &, X87Context &);
struct ContentArguments {
    float center_x, center_y, depth, scale_x, scale_y;
    std::uint8_t cursor;
};
struct DrawCalls {
    virtual ~DrawCalls() = default;
    virtual void select_cursors() = 0;
    virtual void select_panels(texture_lifetime::List &) = 0;
    virtual void set_material(std::uint32_t) = 0;
    virtual void set_base_color(std::uint32_t) = 0;
    virtual void set_vertex_colors(const std::array<std::uint32_t, 4> &) = 0;
    virtual void default_blending() = 0;
    virtual void draw_scaled(std::uint32_t, float x, float y, float depth, float sx, float sy) = 0;
    virtual void draw_shadow_window(float x, float y, float depth, float width, float height) = 0;
    // NOTSA: typed source callback, with the canonical caption state supplied by the session.
    virtual void draw_content(title_menu::DialogContent, const ContentArguments &) = 0;
};
struct DrawState {
    const float &screen_scale_x, &screen_scale_y;
    std::uint32_t &window_after_content;
};
// FUNCTION: SONIC 0x431c30 DrawDialogRound
void draw_round(std::uint32_t, float x, float y, float depth, float sx, float sy, DrawCalls &,
                X87Context &);
// FUNCTION: SONIC 0x431b10 DrawDialogCursorRound
void draw_round_cursor(std::uint8_t alpha, float x, float y, float depth, float sx, float sy,
                       DrawState, DrawCalls &, X87Context &);
// FUNCTION: SONIC 0x431da0 DrawDialogWindowBox
void draw_window_box(float x, float y, float depth, float width, float height, DrawState,
                     DrawCalls &, X87Context &);
// FUNCTION: SONIC 0x431f10 DrawDialogWindow
void draw_window(const Work &, DrawState, DrawCalls &, X87Context &);
// FUNCTION: SONIC 0x431f60 DrawDialogPanels
void draw_panels(const Work &, Owner, DrawState, DrawCalls &, X87Context &);
// FUNCTION: SONIC 0x432480 DisplayDialog
void draw(const Work &, Owner, DrawState, DrawCalls &, X87Context &);
} // namespace sadx::menu_dialog
