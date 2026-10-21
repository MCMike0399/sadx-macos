#include "game/game_view_native.hpp"
#include "cpu/fidelity_float.hpp"
#include "player/player_heading.hpp"

namespace sadx::screen {
using fidelity::ExactFloat;

void set_view_angle(std::int32_t angle, CameraAngleState &state,
                    const std::function<void(std::int32_t)> &set_perspective) {
    if (state.camera_type >= 28 && state.camera_type <= 30)
        angle = state.interpolate == 0 ? 12743 : adjust_angle(state.previous_angle, 12743, 18);
    state.previous_angle = angle;
    set_perspective(angle);
    // The source reloads this owner after the call, rather than copying the argument.
    state.last_angle = state.previous_angle;
}

void begin_frame(std::uint8_t &reset_pending) { reset_pending = 1; }

void end_frame(std::uint8_t &reset_pending, Services &services) {
    reset_pending = 0;
    services.set_depth_compare(3);
    services.set_depth_write(true);
    services.reset_material();
}

void reset_if_pending(State &state, Services &services, X87Context &fp) {
    if (state.reset_pending == 0)
        return;
    services.reset_queue();
    services.set_black_clear_colors();
    services.set_blend(0, 8);
    services.set_blend(1, 6);
    services.reset_material();
    // The source computes the height before the width, each with a binary32 spill.
    const auto height = (ExactFloat(fp, state.vertical_scale) * 480.0f).rounded();
    const auto width = (ExactFloat(fp, state.horizontal_scale) * 640.0f).rounded();
    services.draw_box(0, 0, width, height, -1, 4278190080U);
    state.pause_state = 0;
}

void set_screen(State &state) {
    // SetupScreen ignores the descriptor width and height.
    state.sprite_scale = state.descriptor.distance;
    state.projection_center_x = state.descriptor.center_x;
    state.projection_center_y = state.descriptor.center_y;
}

void set_perspective(std::int32_t angle, State &state, Services &services) {
    if (state.perspective != angle)
        services.set_screen_distance(angle);
    state.perspective_copy = angle;
    state.perspective = angle;
}

void setup(State &state, Services &services, X87Context &fp) {
    services.set_blank_queue_mode();
    reset_if_pending(state, services, fp);
    const auto width = (ExactFloat(fp, state.horizontal_scale) * 640.0f).rounded();
    state.animation_queue_state = 0;
    state.descriptor.width = width;
    state.descriptor.height = (ExactFloat(fp, state.vertical_scale) * 480.0f).rounded();
    state.descriptor.center_x = (ExactFloat(fp, state.descriptor.width) * 0.5f).rounded();
    state.descriptor.center_y = (ExactFloat(fp, state.descriptor.height) * 0.5f).rounded();
    set_screen(state);
    set_perspective(12743, state, services);
    state.last_perspective = 12743;
    // The original computes this rectangle before calling a RET-only function.
    // Preserve its arithmetic side effects, even though no renderer consumes it.
    (void)(ExactFloat(fp, state.horizontal_scale) * 639.0f).rounded();
    (void)(ExactFloat(fp, state.vertical_scale) * 479.0f).rounded();
    services.set_black_clear_colors();
    services.set_control_3d(876);
    services.set_constant_attributes(4294967295U, 8192);
    services.set_material(state.view_color);
    services.set_clip_range(-1, -3000);
    // njSetZMode(1) has no matching case and performs no work.
}
} // namespace sadx::screen
