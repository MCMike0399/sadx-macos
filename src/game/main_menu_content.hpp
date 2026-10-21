#pragma once
#include "game/menu_dialog.hpp"

namespace sadx::main_menu_content {
// NOTSA: typed imported four-float trial backdrop declaration, borrowed by the content callback.
struct Layout {
    float offset_x, offset_y, scale_x, scale_y;
};
struct State {
    const float &screen_scale_x, &screen_scale_y;
    const Layout &trial_layout;
};
struct Calls {
    virtual ~Calls() = default;
    virtual void select_common_texture() = 0;
    virtual void set_material(std::uint32_t) = 0;
    virtual bool optional_entry_available() = 0;
    virtual void draw_scaled(std::uint32_t, float x, float y, float depth, float sx, float sy) = 0;
};
// FUNCTION: SONIC 0x50b4e0 MainMenuDialogContent
void draw_main(const menu_dialog::ContentArguments &, State, Calls &, X87Context &);
// FUNCTION: SONIC 0x50b590 TrialMenuDialogContent
void draw_trial(const menu_dialog::ContentArguments &, State, Calls &, X87Context &);
} // namespace sadx::main_menu_content
