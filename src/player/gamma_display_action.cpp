#include "player/gamma_display.hpp"

namespace sadx::gamma_display {
void ordinary_body(Context &context, std::uint16_t animation) {
    // Source loads the frame before re-reading the mode and selecting the action.
    const auto frame = animation_frame(context);
    if (animation_mode(context) == 2)
        draw_generated_action(context, frame);
    else
        draw_table_action(context, animation, frame);
    clear_object_callback(context);
    push_current(context);
    const auto recoil = (exact(context, aim_translation(context)) * -2.0f).rounded();
    translate_numeric(context, {0, recoil, 0});
    multiply_arm_matrix(context);
    const auto angle = arm_rotation(context);
    if (angle != 0)
        rotate(context, Axis::Y, static_cast<std::uint16_t>(angle));
    complete_arm(context);
}

void gamma_object_callback(Context &context, ObjectId node) {
    const auto table = object_table(context);
    if (node == object_row(context, table, 12)) {
        const auto angle = aim_rotation(context);
        if (angle != 0)
            rotate(context, Axis::Y, static_cast<std::uint16_t>(angle));
    } else if (node == object_row(context, table, 28)) {
        const auto displacement = exact(context, aim_translation(context));
        const auto doubled = (displacement + displacement).rounded();
        translate_numeric(context, {0, doubled, 0});
    }
}
} // namespace sadx::gamma_display
