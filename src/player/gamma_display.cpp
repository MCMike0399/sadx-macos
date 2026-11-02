#include "player/gamma_display.hpp"

namespace sadx::gamma_display {
namespace {
constexpr std::uint16_t kLaserBlaster = 512; // Upgrades_LaserBlaster
constexpr std::uint16_t kHidden = 2; // Status_OnColli; source suppresses this display body
constexpr std::uint32_t kEnableConstantMaterial = 16;
constexpr std::uint32_t kIgnoreAmbient = 0x00080000;
constexpr std::int32_t kHalfTurn = 32768;

// NOTSA: the two writes select live sibling links, not detached model copies.
void select_body(Context &context, std::uint16_t upgrades) {
    const bool upgraded = (upgrades & kLaserBlaster) != 0;
    const auto first = object_row(context, upgraded ? 16 : 14);
    store_sibling(context, object_row(context, 22), first);
    const auto second = object_row(context, upgraded ? 20 : 18);
    store_sibling(context, object_row(context, 23), second);
    const auto selected = sibling(context, object_row(context, 22));
    store_weld_source(context, child(context, selected));
}

// NOTSA: preserve the original full-word zero guard before truncating the angle.
void rotate_task(Context &context, Axis axis) {
    const auto value = angle(context, axis);
    if (value != 0)
        rotate(context, axis, static_cast<std::uint16_t>(value));
}

// NOTSA: the temporary unit frame transforms the motion's vertical offset.
NinjaVector place_body(Context &context) {
    const NinjaVector local{0, motion_height(context), 0};
    push_unit(context);
    rotate_task(context, Axis::Z);
    rotate_task(context, Axis::X);
    const auto yaw = 0U - static_cast<std::uint32_t>(angle(context, Axis::Y));
    if (yaw != 0)
        rotate(context, Axis::Y, static_cast<std::uint16_t>(yaw));
    auto offset = transform_vector(context, local);
    pop(context);
    const auto saved_offset = offset;
    offset.x = (exact(context, offset.x) + position(context, Axis::X)).rounded();
    offset.y = (exact(context, offset.y) + position(context, Axis::Y)).rounded();
    offset.z = (exact(context, offset.z) + position(context, Axis::Z)).rounded();
    translate(context, offset);
    capture_collision_info(context);
    store_collision_center(context, 0, Axis::X, offset.x);
    store_collision_center(context, 0, Axis::Y, offset.y);
    store_collision_center(context, 0, Axis::Z, offset.z);
    store_collision_center(context, 1, Axis::X, offset.x);
    const auto lower_y = (exact(context, position(context, Axis::Y)) + 5.0f).rounded();
    store_collision_center(context, 1, Axis::Y, lower_y);
    store_collision_center(context, 1, Axis::Z, offset.z);
    rotate_task(context, Axis::Z);
    rotate_task(context, Axis::X);
    const auto facing = static_cast<std::uint32_t>(kHalfTurn) -
                        static_cast<std::uint32_t>(angle(context, Axis::Y));
    if (facing != 0)
        rotate(context, Axis::Y, static_cast<std::uint16_t>(facing));
    return saved_offset;
}
} // namespace

void display(Context &context, const TaskHandle &task) {
    capture(context, task);
    const auto upgrades = equipment(context);
    capture_extra(context);
    select_body(context, upgrades);
    select_palette(context, 4);
    auto animation = requested_animation(context);
    if (animation_mode(context) == 2)
        animation = current_animation(context);
    if ((status(context) & kHidden) != 0)
        hidden_tail(context);
    select_body_textures(context);
    save_control(context);
    set_control(context, kEnableConstantMaterial);
    set_white_material(context);
    save_attributes(context);
    clear_attributes(context, 0, kIgnoreAmbient);
    push_current(context);
    const auto offset = place_body(context);
    // Later attachment placement consumes this exact local, after the body draw.
    (void)offset;
    const auto request = requested_animation(context);
    if (request == 46 || request == 52)
        walking_sway(context);
    store_walk_phase(context, 0);
    if (has_event_action(context))
        event_model(context);
    set_object_callback(context);
    draw_player_action(context, animation);
}
} // namespace sadx::gamma_display
