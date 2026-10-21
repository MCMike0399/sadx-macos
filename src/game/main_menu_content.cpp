#include "game/main_menu_content.hpp"
#include "cpu/fidelity_float.hpp"

namespace sadx::main_menu_content {
using fidelity::ExactFloat;

void draw_main(const menu_dialog::ContentArguments &args, State state, Calls &calls,
               X87Context &fp) {
    calls.select_common_texture();
    calls.set_material(4294967295U);
    const float height = calls.optional_entry_available() ? 4.669000148773193f : 4.069000244140625f;
    const float scale_y = (ExactFloat(fp, height) * 0.25f * args.scale_y).rounded();
    const float scale_x = (ExactFloat(fp, args.scale_x) * 0.99225f).rounded();
    const float depth = (ExactFloat(fp, args.depth) - -0.3f).rounded();
    const auto offset_y = ExactFloat(fp, args.scale_y) * 10.0f + args.center_y - 240.0f;
    const auto screen_y = ExactFloat(fp, state.screen_scale_y) * 240.0f;
    const float y = (offset_y + screen_y).rounded();
    const auto offset_x = ExactFloat(fp, args.scale_x) * 10.0f + args.center_x - 320.0f;
    const auto screen_x = ExactFloat(fp, state.screen_scale_x) * 320.0f;
    const float x = (offset_x + screen_x).rounded();
    calls.draw_scaled(67108870U, x, y, depth, scale_x, scale_y);
}

void draw_trial(const menu_dialog::ContentArguments &args, State state, Calls &calls,
                X87Context &fp) {
    calls.select_common_texture();
    calls.set_material(4294967295U);
    const float scale_y = (ExactFloat(fp, state.trial_layout.scale_y) * args.scale_y).rounded();
    const float scale_x = (ExactFloat(fp, state.trial_layout.scale_x) * args.scale_x).rounded();
    const float depth = (ExactFloat(fp, args.depth) - -0.3f).rounded();
    const auto offset_y =
        ExactFloat(fp, state.trial_layout.offset_y) * args.scale_y + args.center_y - 240.0f;
    const auto screen_y = ExactFloat(fp, state.screen_scale_y) * 240.0f;
    const float y = (offset_y + screen_y).rounded();
    const auto offset_x =
        ExactFloat(fp, state.trial_layout.offset_x) * args.scale_x + args.center_x - 320.0f;
    const auto screen_x = ExactFloat(fp, state.screen_scale_x) * 320.0f;
    const float x = (offset_x + screen_x).rounded();
    calls.draw_scaled(67108870U, x, y, depth, scale_x, scale_y);
}
} // namespace sadx::main_menu_content
