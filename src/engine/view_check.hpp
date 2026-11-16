#pragma once
#include "cpu/fidelity_float.hpp"
#include "math/ninja_math.hpp"

#include <cstdint>
#include <functional>

namespace sadx::view_check {
// NOTSA: the live view facts dsCheckViewV reads, borrowed from their owners.
struct State {
    std::int32_t pending_logic_ticks = 0;
    float draw_minimum = 0, draw_maximum = 0;
    float screen_distance = 0, screen_center_x = 0, screen_center_y = 0;
    float ratio_x = 0, ratio_y = 0, multi_x = 0, multi_y = 0;
};
struct Services {
    // njCalcPoint over the live current matrix.
    std::function<NinjaVector(const NinjaVector &)> calculate_point;
};
// FUNCTION: SONIC 0x403330 dsCheckViewV
bool check_view(const NinjaVector &point, float radius, const State &, const Services &,
                fidelity::Context &);
} // namespace sadx::view_check
