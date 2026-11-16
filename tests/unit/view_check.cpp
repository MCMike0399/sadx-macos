#include "engine/view_check.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool value, const std::string &message) {
    if (!value)
        throw std::runtime_error(message);
}
sadx::view_check::State base_state() {
    sadx::view_check::State state;
    state.draw_minimum = -1.0f;
    state.draw_maximum = -2049.0f;
    state.screen_distance = 320.0f;
    state.screen_center_x = 320.0f;
    state.screen_center_y = 240.0f;
    state.ratio_x = 1.0f;
    state.ratio_y = 1.0f;
    state.multi_x = 1.0f;
    state.multi_y = 0.85f;
    return state;
}
bool visible(const sadx::NinjaVector &local, float radius, const sadx::view_check::State &state,
             bool *called = nullptr) {
    sadx::fidelity::Context arithmetic{0x007f};
    sadx::view_check::Services services{[&](const sadx::NinjaVector &) {
        if (called)
            *called = true;
        return local;
    }};
    return sadx::view_check::check_view({400, 0, -20}, radius, state, services, arithmetic);
}
} // namespace

int main() {
    bool called = false;
    require(visible({0, 0, 100}, 25, base_state(), &called) && called, "centered point visible");
    auto pending = base_state();
    pending.pending_logic_ticks = 1;
    require(!visible({0, 0, 100}, 25, pending), "pending ticks cull without transforming");
    require(!visible({0, 0, 3000}, 25, base_state()), "beyond far culled");
    require(!visible({0, 0, -3000}, 25, base_state()), "behind culled");
    require(visible({0, 0, NAN}, 25, base_state()), "unordered depth stays visible");
    require(!visible({-500, 0, 100}, 25, base_state()), "left of screen culled");
    require(visible({0, 0, 100}, 25, base_state()), "center remains visible");
    std::cout << "view check: pending, depth and screen bounds\n";
    return 0;
}
