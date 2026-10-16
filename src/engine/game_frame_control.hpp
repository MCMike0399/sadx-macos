#pragma once
#include <cstdint>
#include <functional>
#include <optional>

namespace sadx::frame {
// NOTSA: plain session state. The reference adapter owns mapped global identities.
// Zero fields are source zero-filled data; the two nonzero settings are file-backed literals.
struct ModeState {
    std::int32_t frame_multiplier = 0;
    std::int32_t logic_increment = 0;
    std::int32_t pending_logic_ticks = 0;
};
struct InputState {
    std::uint8_t &reset_key_edge;
    std::uint8_t soft_reset_flags = 0;
    std::uint8_t reset_input_mask = 0;
    std::int32_t &save_phase;
};
using InputCallback = void (*)(InputState &);
struct RendererState {
    std::int64_t counter_frequency = 0;
    std::int32_t frame_interval_multiplier = 1;
    std::int32_t configured_frame_interval = 3;
    std::int64_t frame_interval = 0;
    std::int64_t previous_counter = 0;
    InputCallback input_callback = nullptr;
};
// An absent value means the OS counter query failed, not a missing fixture word.
struct LimiterClock {
    std::function<std::optional<std::int64_t>()> frequency;
    std::function<std::optional<std::int64_t>()> counter;
    std::function<void(std::int64_t)> wait_hint = {};
};
enum class LimiterFailure { None, MissingClock, CounterUnavailable, DivideFault };
struct LimiterResult {
    LimiterFailure failure = LimiterFailure::None;
    std::optional<std::int64_t> interval{}; // no computed interval on the initialization path/fault
    std::uint64_t counter_reads = 0;
    bool frequency_available = false;
    bool completed = false;
    explicit operator bool() const noexcept { return failure == LimiterFailure::None && completed; }
};
// FUNCTION: SONIC 0x7899a0 stHWSetVSync
void set_frame_interval_multiplier(std::int32_t, RendererState &);
// FUNCTION: SONIC 0x411e30 dsInitInt
void set_frame_rate_mode(std::int32_t render_ratio, std::int32_t logic_ratio, ModeState &,
                         RendererState &);
// FUNCTION: SONIC 0x40f010 UpdateSoftResetFlags
void update_soft_reset_flags(InputState &);
// FUNCTION: SONIC 0x411e20 RunFrameInput
void run_frame_input(InputState &);
// PATCH: SPEEDRUN +0x10f0 -> 0x7899e0
LimiterResult run_speedrun_frame_limiter(RendererState &, const ModeState &, std::int32_t cadence,
                                         const LimiterClock &);
} // namespace sadx::frame
