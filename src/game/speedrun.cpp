#include "game/speedrun.hpp"

#include <limits>

namespace sadx::speedrun {

std::uint32_t reset_camera_flags(std::uint32_t existing_flags) noexcept {
    return existing_flags | camera_reset_bits;
}

SkipCheck in_game_skip_check(std::uint32_t held_buttons,
                             std::uint32_t pressed_buttons) noexcept {
    return {(held_buttons & button_start) != 0,
            pressed_buttons | button_start};
}

bool pre_rendered_skip_check(std::uint32_t held_buttons) noexcept {
    return (held_buttons & (button_start | button_a)) != 0;
}

std::int32_t extra_game_ticks(std::int32_t frame_multiplier) noexcept {
    const auto reduced = static_cast<std::int32_t>(static_cast<std::uint32_t>(frame_multiplier) - 1U);
    return reduced > 0 ? reduced : 0;
}

std::optional<std::int64_t> limiter_interval(std::int32_t frame_multiplier,
    std::int64_t frequency, std::int32_t cadence) noexcept {
    if (cadence == 0) return std::nullopt;
    const auto product = static_cast<std::int64_t>(
        static_cast<std::uint64_t>(static_cast<std::int64_t>(frame_multiplier)) *
        static_cast<std::uint64_t>(frequency));
    if (product == std::numeric_limits<std::int64_t>::min() && cadence == -1) return product;
    return product / cadence;
}

bool limiter_elapsed(std::int64_t now, std::int64_t last, std::int64_t interval) noexcept {
    const auto elapsed = static_cast<std::int64_t>(static_cast<std::uint64_t>(now) - static_cast<std::uint64_t>(last));
    return elapsed >= interval;
}

} // namespace sadx::speedrun
