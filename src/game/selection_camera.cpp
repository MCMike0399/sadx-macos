#include "game/selection_camera.hpp"

namespace sadx::selection_camera {
void update(Pose pose, view::Camera &camera, NinjaMatrixPrimitives &matrices,
            Calls &calls, X87Context &fp) {
    matrices.push_unit_frame();
    const auto yaw = static_cast<std::uint16_t>(pose.rotation[1]);
    if (yaw != 0)
        matrices.rotate_y(matrices.current_matrix(), yaw, fp);
    const auto pitch = static_cast<std::uint16_t>(pose.rotation[0]);
    if (pitch != 0)
        matrices.rotate_x(matrices.current_matrix(), pitch, fp);
    const auto roll = static_cast<std::uint16_t>(ninja_wrapped_negation(pose.rotation[2]));
    if (roll != 0)
        matrices.rotate_z(matrices.current_matrix(), roll, fp);
    NinjaVector direction{};
    const auto result = matrices.calculate_vector(matrices.current_matrix(), {0, 0, -1}, direction, fp);
    if (!result)
        throw CoordinateError(result.failure);
    matrices.end_frame();
    camera.roll = static_cast<std::int32_t>(0U - static_cast<std::uint32_t>(pose.rotation[2]));
    camera.position = {pose.position.x, pose.position.y, pose.position.z};
    camera.direction = {direction.x, direction.y, direction.z};
    calls.set_camera(camera);
    // Source tail-calls 0x51f680, the verified no-op.
}
} // namespace sadx::selection_camera
