#pragma once
#include "cpu/x87_math.hpp"
#include "game/game_view_initial_state.hpp"
#include <functional>

namespace sadx::screen {
// NOTSA: live native owners shared with the renderer and the outer game loop.
// Descriptor dimensions and the renderer's projection width are separate source fields.
struct State {
    const float &horizontal_scale;
    const float &vertical_scale;
    std::uint8_t &reset_pending;
    std::uint32_t &pause_state;
    std::int32_t &animation_queue_state;
    GameViewInitialScreen &descriptor;
    float &sprite_scale;
    float &projection_center_x;
    float &projection_center_y;
    std::int32_t &perspective;
    std::int32_t &perspective_copy;
    std::int32_t &last_perspective;
    const std::array<float, 4> &view_color;
};
struct Services {
    std::function<void()> set_blank_queue_mode;
    std::function<void()> reset_queue;
    std::function<void()> set_black_clear_colors;
    std::function<void(std::uint32_t, std::uint32_t)> set_blend;
    std::function<void()> reset_material;
    std::function<void(float, float, float, float, float, std::uint32_t)> draw_box;
    std::function<void(std::int32_t)> set_screen_distance;
    std::function<void(std::uint32_t)> set_control_3d;
    std::function<void(std::uint32_t, std::uint32_t)> set_constant_attributes;
    std::function<void(const std::array<float, 4> &)> set_material;
    std::function<void(float, float)> set_clip_range;
    std::function<void(std::uint32_t)> set_depth_compare;
    std::function<void(bool)> set_depth_write;
};
// NOTSA: the gameplay-camera selection and previous FOV are distinct from Ninja View.
struct CameraAngleState {
    const std::int8_t &camera_type;
    const std::uint8_t &interpolate;
    std::int32_t &previous_angle;
    std::int32_t &last_angle;
};
// FUNCTION: SONIC 0x437240 SetViewAngle
void set_view_angle(std::int32_t, CameraAngleState &,
                    const std::function<void(std::int32_t)> &set_perspective);
// FUNCTION: SONIC 0x420830 set_byte_3B280A8_1
void begin_frame(std::uint8_t &reset_pending);
// FUNCTION: SONIC 0x420840 set_byte_3B280A8_0
void end_frame(std::uint8_t &reset_pending, Services &);
// FUNCTION: SONIC 0x420860 FUN_00420860
void reset_if_pending(State &, Services &, X87Context &);
// FUNCTION: SONIC 0x788240 SetupScreen
void set_screen(State &);
// FUNCTION: SONIC 0x402ed0 njSetPerspective
void set_perspective(std::int32_t angle, State &, Services &);
// FUNCTION: SONIC 0x4208e0 SetupSomeScreenStuff
void setup(State &, Services &, X87Context &);
} // namespace sadx::screen
