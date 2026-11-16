#include "engine/view_check.hpp"

#include <cstdint>
#include <cstring>

namespace {
enum Input : unsigned {
    PointX = 0, PointY, PointZ, Radius, LocalX, LocalY, LocalZ, Pending,
    DrawMin, DrawMax, Dist, CenterX, CenterY, RatioX, RatioY, MultiX, MultiY,
    ControlWord, Words
};
float float_at(std::uint32_t word) {
    float value;
    std::memcpy(&value, &word, sizeof(value));
    return value;
}
std::uint32_t bits_of(float value) {
    std::uint32_t word;
    std::memcpy(&word, &value, sizeof(word));
    return word;
}
} // namespace

extern "C" int sadx_native_view_check(const std::uint32_t *input, std::uint32_t *output) {
    try {
        sadx::view_check::State state;
        state.pending_logic_ticks = static_cast<std::int32_t>(input[Pending]);
        state.draw_minimum = float_at(input[DrawMin]);
        state.draw_maximum = float_at(input[DrawMax]);
        state.screen_distance = float_at(input[Dist]);
        state.screen_center_x = float_at(input[CenterX]);
        state.screen_center_y = float_at(input[CenterY]);
        state.ratio_x = float_at(input[RatioX]);
        state.ratio_y = float_at(input[RatioY]);
        state.multi_x = float_at(input[MultiX]);
        state.multi_y = float_at(input[MultiY]);
        sadx::NinjaVector captured{};
        bool called = false;
        sadx::view_check::Services services{[&](const sadx::NinjaVector &point) {
            called = true;
            captured = point;
            return sadx::NinjaVector{float_at(input[LocalX]), float_at(input[LocalY]),
                                     float_at(input[LocalZ])};
        }};
        sadx::fidelity::Context arithmetic(static_cast<std::uint16_t>(input[ControlWord]));
        const bool visible = sadx::view_check::check_view(
            {float_at(input[PointX]), float_at(input[PointY]), float_at(input[PointZ])},
            float_at(input[Radius]), state, services, arithmetic);
        std::memset(output, 0, 5 * sizeof(std::uint32_t));
        output[0] = visible ? 1 : 0;
        output[1] = called ? 1 : 0;
        output[2] = bits_of(captured.x);
        output[3] = bits_of(captured.y);
        output[4] = bits_of(captured.z);
        return 1;
    } catch (...) {
        return 0;
    }
}
