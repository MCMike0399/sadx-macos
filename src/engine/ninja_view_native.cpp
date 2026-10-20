#include "engine/ninja_view_native.hpp"
#include "cpu/fidelity_float.hpp"
#include <cstring>

namespace sadx::view {
namespace {
using fidelity::ExactFloat;
// NOTSA: native matrices carry floats; D3DX's SSE implementation retains their exact words.
D3dxMatrix words(const NinjaMatrix &matrix) {
    D3dxMatrix result;
    std::memcpy(result.data(), matrix.data(), sizeof(matrix));
    return result;
}
// NOTSA: the original unsigned FILD sequence includes the rounded addition for negative words.
ExactFloat unsigned_number(std::uint32_t number, X87Context &fp) {
    auto value = ExactFloat::integer(fp, static_cast<std::int32_t>(number));
    if (static_cast<std::int32_t>(number) < 0)
        value = value + 4294967296.0f;
    return value;
}
}
void initialize_camera(Camera &camera) {
    camera.position = {0, 0, 0};
    camera.direction = {0, 0, -1};
    camera.roll = 0;
    camera.angle = 10922;
    camera.near_distance = -1;
    camera.far_distance = -60000;
    camera.tail = {1, 0, 0, 0, 1, 0};
}
Result set_projection(float fov, float far_distance, State &state, Services &services,
                       X87Context &fp, SseContext &sse) {
    const auto width = unsigned_number(state.depth.viewport.width, fp);
    const auto height = unsigned_number(state.depth.viewport.height, fp);
    const auto aspect = (width / height).rounded();
    const auto tangent = (ExactFloat(fp, fov) * 0.5f).tangent();
    if (!tangent)
        return {Failure::TangentOutOfRange, false};
    const auto scaled = *tangent / aspect;
    const auto one = ExactFloat(fp, 1.0f);
    const auto angle = ExactFloat::arctangent(scaled, one);
    const auto vertical_fov = (angle + angle).rounded();
    const auto perspective = d3dx_perspective_7716df(vertical_fov, aspect, 1, far_distance, fp);
    if (!perspective)
        return {Failure::PerspectiveOutOfRange, false};
    state.perspective = *perspective;
    const auto flip = words({1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});
    state.depth.projection = d3dx_matrix_multiply_774310(flip, *perspective, sse);
    services.transform(false, 3, state.depth.projection);
    services.transform(true, 3, services.screen_transform);
    const auto current = words(services.matrices.current());
    services.transform(false, 2, current);
    state.device_view = current;
    const auto inverse = d3dx_matrix_inverse_7757e0(current, sse, services.reciprocal);
    if (inverse.matrix)
        state.inverse = *inverse.matrix;
    return {Failure::None, bool(inverse.matrix)};
}
Result set_camera(State &state, Services &services, X87Context &fp, SseContext &sse) {
    NinjaMath math(services.sine, fp);
    const auto roll = static_cast<std::int32_t>(0U - static_cast<std::uint32_t>(state.camera.roll));
    const auto sine = math.sin(roll);
    const auto cosine = math.cos(roll);
    const auto z = ExactFloat(fp, state.camera.direction[2]);
    const auto x = ExactFloat(fp, state.camera.direction[0]);
    const auto horizontal_squared = (x * x + z * z).rounded();
    const auto horizontal = ExactFloat::signed_square_root(fp, horizontal_squared).rounded();
    const auto y = ExactFloat(fp, state.camera.direction[1]);
    const auto length_squared = (y * y + horizontal_squared).rounded();
    const auto inverse = ExactFloat::signed_square_root(fp, length_squared).reciprocal_double(1.0);
    const auto cos_pitch = (ExactFloat(fp, horizontal) * inverse).rounded();
    const auto sin_pitch = (inverse * state.camera.direction[1]).rounded();
    auto yaw_sine = ExactFloat(fp, 0.0f), yaw_cosine = yaw_sine;
    if (ExactFloat(fp, horizontal_squared).compare(0.000001f) == X87Comparison::greater) {
        const auto inverse_horizontal =
            ExactFloat::signed_square_root(fp, horizontal_squared).reciprocal_double(1.0);
        const auto stored_inverse = inverse_horizontal.rounded();
        yaw_cosine = inverse_horizontal * state.camera.direction[2];
        yaw_sine = -(ExactFloat(fp, stored_inverse) * state.camera.direction[0]);
    } else {
        yaw_cosine = ExactFloat(fp, 1.0f);
        yaw_sine = ExactFloat(fp, 0.0f);
    }
    auto &matrix = state.view;
    const auto pitched_sine = ExactFloat(fp, sin_pitch) * yaw_sine;
    matrix[3] = 0;
    matrix[6] = sin_pitch;
    matrix[7] = 0;
    matrix[11] = 0;
    matrix[0] = (yaw_cosine * cosine - pitched_sine * sine).rounded();
    matrix[1] = (pitched_sine * cosine + yaw_cosine * sine).rounded();
    matrix[2] = (-(ExactFloat(fp, cos_pitch) * yaw_sine)).rounded();
    matrix[4] = (-(ExactFloat(fp, cos_pitch) * sine)).rounded();
    matrix[5] = (ExactFloat(fp, cos_pitch) * cosine).rounded();
    const auto pitched_cosine = yaw_cosine * sin_pitch;
    matrix[8] = (ExactFloat(fp, sine) * pitched_cosine + yaw_sine * cosine).rounded();
    matrix[9] = (yaw_sine * sine - pitched_cosine * cosine).rounded();
    matrix[10] = (yaw_cosine * cos_pitch).rounded();
    const auto translation = [&](float a, float b, float c) {
        const auto first = -(ExactFloat(fp, a) * state.camera.position[2]);
        const auto second = -(ExactFloat(fp, b) * state.camera.position[0]);
        const auto third = -(ExactFloat(fp, c) * state.camera.position[1]);
        return (first + second + third).rounded();
    };
    matrix[12] = translation(matrix[8], matrix[0], matrix[4]);
    matrix[13] = translation(matrix[9], matrix[1], matrix[5]);
    matrix[15] = 1;
    services.matrices.reset_to_first();
    matrix[14] = translation(matrix[10], matrix[2], sin_pitch);
    auto &slot = services.matrices.current();
    for (unsigned row = 0; row < 4; ++row) {
        slot[row * 4] = (-ExactFloat(fp, matrix[row * 4])).rounded();
        slot[row * 4 + 1] = (-ExactFloat(fp, matrix[row * 4 + 1])).rounded();
        slot[row * 4 + 2] = matrix[row * 4 + 2];
        slot[row * 4 + 3] = row == 3 ? 1 : 0;
    }
    state.view_copy = slot;
    const auto screen_width = unsigned_number(state.depth.viewport.width, fp).rounded();
    const auto signed_angle = state.camera.angle;
    const auto half_angle = (signed_angle + (signed_angle < 0 ? 1 : 0)) >> 1;
    const auto half_sine = math.sin(half_angle);
    const auto tangent = ExactFloat(fp, half_sine) /
                         ExactFloat::retained(fp, math.cos_extended(half_angle));
    state.sprite_scale = (ExactFloat(fp, screen_width) / (tangent + tangent)).rounded();
    state.near_distance = (-ExactFloat(fp, state.camera.near_distance)).rounded();
    state.far_distance = (-ExactFloat(fp, state.camera.far_distance)).rounded();
    const auto fov = ExactFloat::integer(fp, signed_angle)
                         .multiply_double(9.5873779296875e-05).rounded();
    return set_projection(fov, state.far_distance, state, services, fp, sse);
}
} // namespace sadx::view

namespace sadx::view {
void clamp_clip_range(float near_distance, float far_distance, State &state, X87Context &fp) {
    if (ExactFloat(fp, near_distance).compare(-1.0f) == X87Comparison::greater)
        near_distance = -1;
    else if (ExactFloat(fp, near_distance).compare(-65535.0f) == X87Comparison::less)
        near_distance = -65535;
    auto far_value = ExactFloat(fp, far_distance);
    if (far_value.compare(-1.0f) == X87Comparison::greater)
        far_value = ExactFloat(fp, -1.0f);
    else if (far_value.compare(-65535.0f) == X87Comparison::less)
        far_value = ExactFloat(fp, -65535.0f);
    state.near_distance = (-ExactFloat(fp, near_distance)).rounded();
    state.far_distance = (-far_value).rounded();
}
bool set_clip_range(float near_distance, float far_distance, State &state,
                    ClipServices &services, X87Context &fp) {
    clamp_clip_range(near_distance, far_distance, state, fp);
    const auto near_value = -ExactFloat(fp, state.near_distance);
    const auto far_value = (-ExactFloat(fp, state.far_distance)).rounded();
    state.negative_near_distance = near_value.rounded();
    state.negative_far_distance = far_value;
    // The source's negative-distance cache is the live camera's near/far pair.
    bool changed = ExactFloat(fp, state.camera.near_distance).compare(near_value) !=
                   X87Comparison::equal;
    if (!changed)
        changed = ExactFloat(fp, state.camera.far_distance).compare(far_value) !=
                  X87Comparison::equal;
    if (!changed)
        return false;
    state.camera.near_distance = near_value.rounded();
    const auto positive_near = (-near_value).rounded();
    const auto positive_far_value = -ExactFloat(fp, far_value);
    state.camera.far_distance = far_value;
    const auto positive_far = positive_far_value.rounded();
    auto projection = services.get_projection();
    const auto span = ExactFloat(fp, positive_far) - positive_near;
    const auto quotient = positive_far / span;
    projection[10] = quotient.rounded();
    projection[14] = (-(quotient * positive_near)).rounded();
    services.set_projection(projection);
    return true;
}
} // namespace sadx::view
