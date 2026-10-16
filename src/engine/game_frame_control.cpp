#include "engine/game_frame_control_internal.hpp"
#include "engine/speedrun.hpp"
#include <cstring>

namespace sadx::frame {
void set_frame_interval_multiplier(std::int32_t value, RendererState &state) {
    const auto frequency = counter_frequency(state);
    store_interval_multiplier(state, value);
    // The original multiply keeps the low 64 bits, then divides the signed bit pattern.
    const auto product = std::uint64_t(std::int64_t(value)) * std::uint64_t(frequency);
    std::int64_t signed_product;
    std::memcpy(&signed_product, &product, sizeof(signed_product));
    store_frame_interval(state, signed_product / 60);
}
void set_frame_rate_mode(std::int32_t render_ratio, std::int32_t logic_ratio, ModeState &timing,
                         RendererState &renderer) {
    store_frame_multiplier(timing, render_ratio);
    store_logic_increment(timing, logic_ratio);
    clear_pending_logic_ticks(timing);
    if (render_ratio == 1) {
        const auto value = configured_frame_interval(renderer, ModeInterval::Normal);
        apply_mode_interval(renderer, value, ModeInterval::Normal);
    } else if (render_ratio == 2 &&
               configured_frame_interval(renderer, ModeInterval::Double) == 1) {
        apply_mode_interval(renderer, 2, ModeInterval::Double);
    }
    store_input_callback(renderer, &run_frame_input);
}
void update_soft_reset_flags(InputState &state) {
    const auto flags = soft_reset_flags(state);
    if (flags != 0) {
        if (save_phase(state) == 0)
            store_soft_reset_flags(state, flags | 1U);
        return;
    }
    constexpr std::uint8_t kResetInputMask = 68;
    if (reset_key_edge(state) != 0 && (reset_input_mask(state) & kResetInputMask) != 0) {
        store_soft_reset_flags(state, flags | 4U);
        return;
    }
    store_soft_reset_flags(state, flags & 251U);
}
void run_frame_input(InputState &state) { update_soft_reset_flags(state); }

LimiterResult run_speedrun_frame_limiter(RendererState &renderer, const ModeState &timing,
                                         std::int32_t cadence, const LimiterClock &clock) {
    LimiterResult result{};
    if (!clock.frequency || !clock.counter) {
        result.failure = LimiterFailure::MissingClock;
        return result;
    }
    const auto frequency = clock.frequency();
    if (!frequency) {
        result.counter_reads = 1;
        const auto now = clock.counter();
        if (!now) {
            result.failure = LimiterFailure::CounterUnavailable;
            return result;
        }
        store_previous_counter(renderer, *now);
        result.completed = true;
        return result;
    }
    result.frequency_available = true;
    store_counter_frequency(renderer, *frequency);
    report_limiter_progress(result);
    result.interval = speedrun::limiter_interval(frame_multiplier(timing), *frequency, cadence);
    if (!result.interval) {
        result.failure = LimiterFailure::DivideFault;
        return result;
    }
    for (;;) {
        const auto now = clock.counter();
        ++result.counter_reads;
        report_limiter_progress(result);
        if (!now) {
            result.failure = LimiterFailure::CounterUnavailable;
            return result;
        }
        const auto previous = previous_counter(renderer);
        if (speedrun::limiter_elapsed(*now, previous, *result.interval)) {
            store_previous_counter(renderer, *now);
            break;
        }
        if (clock.wait_hint) {
            const auto elapsed =
                static_cast<std::int64_t>(std::uint64_t(*now) - std::uint64_t(previous));
            clock.wait_hint(*result.interval - elapsed);
        }
    }
    result.completed = true;
    return result;
}
} // namespace sadx::frame
