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
constexpr std::uint32_t kProbeLiftBits = 0x40800000u;
constexpr std::uint32_t kGapScaleBits = 0x3cf5c28fu;
constexpr std::uint32_t kGroundLiftBits = 0x3e99999au;
constexpr std::uint32_t kFlattenBits = 0x3f4ccccdu;
const float kProbeLift = image_float(kProbeLiftBits);
const float kGapScale = image_float(kGapScaleBits);
const float kGroundLift = image_float(kGroundLiftBits);
const float kFlatten = image_float(kFlattenBits);

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
