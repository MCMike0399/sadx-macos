#include "player/gamma_display.hpp"
#include <algorithm>
#include <cstring>

namespace sadx::gamma_display {
namespace {
// NOTSA: keep the full angle guard before narrowing the Ninja angle.
void rotate_attachment(Context &context, Axis axis) {
    const auto value = angle(context, axis);
    if (value != 0)
        rotate(context, axis, static_cast<std::uint16_t>(value));
}
// NOTSA: world placement for the source's attachment calculations, separate from the draw frame.
void place_attachments(Context &context, const NinjaVector &offset) {
    push_unit(context);
    translate(context, {position(context, Axis::X), position(context, Axis::Y), position(context, Axis::Z)});
    translate(context, offset);
    rotate_attachment(context, Axis::Z);
    rotate_attachment(context, Axis::X);
    const auto yaw = std::uint32_t(32768) - std::uint32_t(angle(context, Axis::Y));
    if (yaw != 0)
        rotate(context, Axis::Y, static_cast<std::uint16_t>(yaw));
}
// NOTSA: hip points share the source scratch vector, including its untouched Y component.
void update_hip_points(Context &context) {
    push_current(context);
    multiply_attachment(context, Attachment::Hip);
    auto &point = display_vector(context);
    point.x = 0;
    point.z = 6;
    store_extra_vector(context, ExtraVector::HipLeft, calculate_point(context, point));
    point.z = -point.z;
    store_extra_vector(context, ExtraVector::HipRight, calculate_point(context, point));
    point.z = 0;
    pop(context);
}
// NOTSA: ordinary laser aiming; target and player-directed branches stop explicitly.
void update_arm_points(Context &context) {
    push_current(context);
    if (laser_mode(context) == 3)
        laser_target(context);
    if (!paused(context)) {
        const auto old = std::uint32_t(arm_rotation(context));
        const auto low = static_cast<std::uint16_t>(0U - old);
        std::int16_t delta;
        std::memcpy(&delta, &low, sizeof(delta));
        const auto step = std::clamp(std::int32_t(delta), -1456, 1456);
        const auto updated = old + std::uint32_t(step);
        std::int32_t signed_updated;
        std::memcpy(&signed_updated, &updated, sizeof(updated));
        store_arm_rotation(context, signed_updated);
    }
    multiply_arm_matrix(context);
    const auto rotation = arm_rotation(context);
    if (rotation != 0)
        rotate(context, Axis::Y, static_cast<std::uint16_t>(rotation));
    multiply_attachment(context, Attachment::Arm);
    auto &point = display_vector(context);
    store_extra_vector(context, ExtraVector::LaserPosition, calculate_point(context, point));
    if (laser_lock_timer(context) > 0)
        laser_target(context);
    point.x = -1;
    store_extra_vector(context, ExtraVector::LaserDirection, transform_vector(context, point));
    pop(context);
}
// NOTSA: the gun direction is normalized after its vector store in the original.
void update_gun_points(Context &context) {
    push_current(context);
    multiply_attachment(context, Attachment::Gun);
    auto &point = display_vector(context);
    point.x = 0;
    store_extra_vector(context, ExtraVector::GunPosition, calculate_point(context, point));
    point.x = 1;
    auto direction = transform_vector(context, point);
    store_extra_vector(context, ExtraVector::GunDirection, direction);
    normalize(context, direction);
    store_extra_vector(context, ExtraVector::GunDirection, direction);
    pop(context);
}
} // namespace

// NOTSA: shared arm-call and matrix-producer suffix of the ordinary body.
void complete_arm(Context &context) {
    draw_arm_action(context);
    calculate_arm_matrix(context);
    pop(context);
}

// NOTSA: source ordinary tail after the arm; active branches keep named stops.
void display_tail(Context &context, const NinjaVector &offset) {
    if ((status(context) & 1024U) != 0)
        attachment_effect(context);
    const auto gun = gun_mode(context);
    if (gun >= 2 && gun <= 5)
        gun_action(context);
    const auto laser = laser_mode(context);
    if (laser >= 2 && laser <= 3)
        gun_action(context);
    clear_display_flags(context);
    place_attachments(context, offset);
    update_hip_points(context);
    update_arm_points(context);
    update_gun_points(context);
    pop(context);
    // Source pushes the saved node matrix directly, without multiplying the caller frame.
    push_arm_matrix(context);
    const auto origin = calculate_point(context, {0, 0, 0});
    store_eye_height(context, (exact(context, origin.y) + 15.0f).rounded());
    pop(context);
    pop(context);
    if (!paused(context))
        store_headlight_timer(context, headlight_timer(context) + 1024U);
    auto brightness = sine(context, static_cast<std::int32_t>(headlight_timer(context))) * 255.0f;
    if (brightness.is_negative())
        brightness = -brightness;
    store_headlight_alpha(context, static_cast<std::uint8_t>(brightness.truncated_integer()));
    if (requested_animation(context) == 69) {
        const auto comparison = exact(context, animation_frame(context)).compare(40.0f);
        if (comparison != X87Comparison::less && comparison != X87Comparison::unordered)
            headlight_fade(context);
    }
    restore_attributes(context);
    reset_material(context);
    restore_control(context);
    select_palette(context, 0);
    if (paused(context))
        paused_shadow(context);
}

} // namespace sadx::gamma_display
