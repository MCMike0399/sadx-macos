#include "player/gamma_fan.hpp"
#include <algorithm>
#include <cstring>

namespace sadx::gamma_fan {
void update(Access &access, fidelity::Context &arithmetic) {
    auto speed = fidelity::ExactFloat(arithmetic, access.lateral_speed());
    speed = speed + access.forward_speed();
    if (speed.is_negative())
        speed = -speed;
    speed = speed - 1.0f;
    if (speed.compare(0.0f) == X87Comparison::less) {
        speed = fidelity::ExactFloat(arithmetic, 0.0f);
    } else if (speed.compare(1.0f) == X87Comparison::greater) {
        speed = fidelity::ExactFloat(arithmetic, 1.0f);
    }
    const auto phase_step = (speed * 12.5f)
                                .multiply_double(65536.0)
                                .multiply_double(-0.002777777777777778)
                                .truncated_integer();
    // Original integer addition wraps; the full word is retained between frames.
    const auto phase = access.phase() + 2048U - static_cast<std::uint32_t>(phase_step);
    access.store_phase(phase);
    std::int32_t angle;
    std::memcpy(&angle, &phase, sizeof(angle));
    access.store_height(access.sine(angle).rounded());

    const auto tilt_value = (fidelity::ExactFloat(arithmetic, access.forward_speed()) * 10.0f)
                                .multiply_double(65536.0)
                                .multiply_double(0.002777777777777778)
                                .truncated_integer();
    const auto tilt_bits = static_cast<std::uint32_t>(tilt_value);
    std::int32_t tilt;
    std::memcpy(&tilt, &tilt_bits, sizeof(tilt));
    access.store_tilt(std::clamp(tilt, -4551, 4551));
}
} // namespace sadx::gamma_fan
