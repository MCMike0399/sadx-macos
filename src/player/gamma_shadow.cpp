#include "player/gamma_shadow.hpp"

#include <cstdint>
#include <cstring>

namespace sadx::gamma_shadow {
namespace {
float image_float(std::uint32_t bits) {
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
const float kProbeLift = image_float(0x40800000u);   // 0x7dcc74
const float kGapScale = image_float(0x3cf5c28fu);    // 0x7e00d0
const float kGroundLift = image_float(0x3e99999au);  // 0x7dd540
const float kFlatten = image_float(0x3f4ccccdu);     // 0x7dce5c

std::uint16_t low_negation(std::int32_t yaw) {
    return static_cast<std::uint16_t>(-static_cast<std::uint16_t>(yaw));
}
} // namespace

fidelity::ExactFloat update(const Body &body, float scale, Services &services,
                            fidelity::Context &arithmetic) {
    const auto raised = (fidelity::ExactFloat(arithmetic, body.y) + kProbeLift).rounded();
    const auto ground = services.sample_ground(body.x, raised, body.z);
    const auto height = fidelity::ExactFloat(arithmetic, ground.height);
    if (services.missed_frames())
        return height;
    const auto gap = fidelity::ExactFloat(
        arithmetic, fidelity::absolute_float((fidelity::ExactFloat(arithmetic, body.y) - height).rounded()));
    const auto denom = gap * kGapScale + 1.0f;
    const auto factor = (fidelity::ExactFloat(arithmetic, scale) / denom).rounded();
    services.prepare();
    services.push_matrix();
    const auto lifted = (height + kGroundLift).rounded();
    services.translate(body.x, lifted, body.z);
    if (ground.normal_z != 0)
        services.rotate_z(static_cast<std::uint16_t>(ground.normal_z));
    if (ground.normal_x != 0)
        services.rotate_x(static_cast<std::uint16_t>(ground.normal_x));
    if (body.yaw != 0)
        services.rotate_y(low_negation(body.yaw));
    const auto flat = (fidelity::ExactFloat(arithmetic, factor) * kFlatten).rounded();
    services.scale(factor, flat, factor);
    services.draw(factor);
    services.pop_matrix();
    return height;
}
} // namespace sadx::gamma_shadow
