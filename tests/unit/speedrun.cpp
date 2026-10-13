#include "game/speedrun.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main() {
    using namespace sadx::speedrun;
    // CCEF must retain the chosen free-camera bit across both reset paths.
    require(reset_camera_flags(1) == 0x80000005, "free-camera choice lost");
    require(reset_camera_flags(0x2041) == 0x80002045,
            "unrelated camera flags lost");
    require(reset_camera_flags(reset_camera_flags(1)) == reset_camera_flags(1),
            "camera reset is not idempotent");

    const auto held_start = in_game_skip_check(0x108, 0x200);
    require(held_start.requested && held_start.pressed_buttons == 0x208,
            "held Start must request skip while preserving pressed buttons");
    const auto tap_only = in_game_skip_check(0x104, 0x8);
    require(!tap_only.requested && tap_only.pressed_buttons == 0x8,
            "in-game skip must use held Start rather than A/pressed Start");
    const auto no_start = in_game_skip_check(0, 0x10204);
    require(!no_start.requested && no_start.pressed_buttons == 0x1020c,
            "original hook's unconditional pressed-bit write lost");
    require(pre_rendered_skip_check(0x4) && pre_rendered_skip_check(0x8),
            "pre-rendered skip must accept held A or Start");
    require(!pre_rendered_skip_check(0x10202),
            "unrelated buttons must not skip movies");

    // 63 Hz presentation does not change the separate simulation tick policy.
    // Speedrun 2.0 +0x11c0 / +0x10f0 semantics (see speedrun.hpp Evidence).
    require(extra_game_ticks(1) == 0 && extra_game_ticks(2) == 1 && extra_game_ticks(4) == 3 && extra_game_ticks(0) == 0 && extra_game_ticks(-5) == 0,
            "wrapped decrement floored at zero");
    require(extra_game_ticks(std::numeric_limits<std::int32_t>::min()) == std::numeric_limits<std::int32_t>::max(),
            "INT_MIN-1 wraps positive in the 32-bit hook");
    require(limiter_interval(1, 10'000'000, cadence_63fps) == 158'730 && limiter_interval(2, 10'000'000, cadence_63fps) == 317'460,
            "10 MHz QPC 63 FPS interval truncates");
    require(limiter_interval(1, 1'000'000'000, 60) == 16'666'666 && limiter_interval(-1, 10'000'000, 63) == -158'730,
            "signed truncating divide");
    require(!limiter_interval(1, 1, 0) && limiter_interval(std::numeric_limits<std::int32_t>::min(), std::int64_t{1} << 32, -1) == std::numeric_limits<std::int64_t>::min(),
            "zero cadence faults; INT64_MIN/-1 wraps");
    require(limiter_elapsed(158'730, 0, 158'730) && !limiter_elapsed(158'729, 0, 158'730) && limiter_elapsed(5, 10, -5) && !limiter_elapsed(5, 10, -4),
            "signed 64-bit elapsed comparison");
    require(limiter_elapsed(std::numeric_limits<std::int64_t>::min(), std::numeric_limits<std::int64_t>::max(), 1),
            "wrapped counter difference");
    std::cout << "speedrun semantics passed\n";
}
