#include "game/selection_question.hpp"

namespace sadx::selection_question {
void execute(Pose pose, const std::uint32_t &logic_ticks, Assets assets, Calls &calls) {
    const auto yaw = static_cast<std::uint32_t>(pose.rotation[1]) + logic_ticks * 728U;
    pose.rotation[1] = static_cast<std::int32_t>(yaw);
    calls.push_current_matrix();
    calls.translate_current(pose.position);
    if (pose.rotation[2] != 0)
        calls.rotate_z(static_cast<std::uint16_t>(pose.rotation[2]));
    if (pose.rotation[0] != 0)
        calls.rotate_x(static_cast<std::uint16_t>(pose.rotation[0]));
    if (pose.rotation[1] != 0)
        calls.rotate_y(static_cast<std::uint16_t>(0U - static_cast<std::uint32_t>(pose.rotation[1])));
    calls.enable_specular();
    calls.select_menu_texture(assets.menu_texture);
    calls.draw_object(assets.object);
    calls.pop_matrix(1);
}
} // namespace sadx::selection_question
