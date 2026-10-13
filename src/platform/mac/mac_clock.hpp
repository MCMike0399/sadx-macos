#pragma once
#include "game/game_frame_control.hpp"
#include <cstdint>
namespace sadx {
// macOS counter owner presenting a 10 MHz QueryPerformanceCounter, the
// frequency observed on the Windows reference (QPC 10,000,000 Hz), so the
// Speedrun limiter computes the reference's exact interval (158,730 ticks at
// 63 FPS). Derived from mach_absolute_time with exact 128-bit scaling.
class MacPerformanceCounter {
public:
    static constexpr std::int64_t frequency_hz=10'000'000;
    MacPerformanceCounter();
    std::int64_t now()const noexcept;
    // Limiter clock; the wait hint sleeps until ~4 ms before the deadline and
    // leaves the final ~4 ms to the source spin: the limiter has no catch-up, so
    // any oversleep would slow the game.
    SpeedrunLimiterClock limiter_clock()const;
private:
    std::uint32_t numer_,denom_;
};
} // namespace sadx
