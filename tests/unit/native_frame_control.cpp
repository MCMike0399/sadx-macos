#include "engine/game_frame_control.hpp"
#include "engine/speedrun.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

// This executable owns its counter and input facts. It deliberately links the
// two maintained implementation files directly, without a reference runtime.
struct NativeHost {
    sadx::FrameRateModeState timing;
    sadx::GameFrameRendererState renderer;
    std::optional<std::uint8_t> reset_edge;
    std::optional<std::int32_t> save_phase;
    sadx::GameFrameInputState input{reset_edge, {}, {}, save_phase};
    std::optional<std::int64_t> frequency = 10'000'000;
    std::vector<std::optional<std::int64_t>> counters;
    std::size_t cursor = 0;
    std::vector<std::int64_t> waits;

    sadx::SpeedrunLimiterClock clock() {
        return {[this] { return frequency; }, [this] {
            if (cursor >= counters.size()) throw std::runtime_error("counter script exhausted");
            return counters[cursor++];
        }, [this](std::int64_t remaining) { waits.push_back(remaining); }};
    }
    sadx::SpeedrunLimiterResult frame(std::vector<std::optional<std::int64_t>> samples) {
        counters = std::move(samples);
        cursor = 0;
        waits.clear();
        return sadx::run_speedrun_frame_limiter(renderer, timing,
                                               sadx::speedrun::cadence_63fps, clock());
    }
    sadx::GameFrameControlResult poll_input() {
        require(renderer.input_callback && *renderer.input_callback,
                "mode setup must install a callable native input handler");
        return (*renderer.input_callback)(input);
    }
};

void host_journey() {
    NativeHost host;
    // The failed-frequency initialization path seeds the first counter without
    // assuming a performance frequency or a previous reference counter.
    host.frequency.reset();
    auto frame = host.frame({900});
    require(bool(frame) && !frame.frequency_available && frame.counter_reads == 1 &&
            host.renderer.previous_counter == 900, "native clock initialization");
    host.frequency = 10'000'000;
    host.renderer.configured_frame_interval = 1;
    frame = host.frame({1000});
    require(frame.failure == sadx::SpeedrunLimiterFailure::unknown_frame_multiplier &&
            host.cursor == 0 && host.renderer.counter_frequency.low == 10'000'000U &&
            host.renderer.counter_frequency.high == 0U,
            "clock frequency stores precede an unknown timing-mode failure");
    require(bool(sadx::set_frame_rate_mode(1, 1, host.timing, host.renderer)),
            "native mode setup");
    require(host.timing.frame_multiplier == 1 && host.timing.logic_increment == 1 &&
            host.timing.pending_logic_ticks == 0 && host.renderer.frame_interval_multiplier == 1 &&
            host.renderer.frame_interval.low == 166'666U && host.renderer.frame_interval.high == 0U,
            "mode setup uses source 60 Hz renderer interval");

    host.input.soft_reset_flags = 0;
    host.reset_edge = 1;
    host.input.reset_input_mask = 0x40;
    require(bool(host.poll_input()) && host.input.soft_reset_flags == 4,
            "installed handler consumes host-owned reset input");
    host.save_phase = 2;
    require(bool(host.poll_input()) && host.input.soft_reset_flags == 4,
            "active save phase preserves pending reset");
    host.save_phase = 0;
    host.reset_edge.reset();
    host.input.reset_input_mask.reset();
    require(bool(host.poll_input()) && host.input.soft_reset_flags == 5,
            "finished save raises soft reset without requiring unused input facts");

    frame = host.frame({159'629, 159'630});
    require(bool(frame) && frame.interval == 158'730 && frame.counter_reads == 2 &&
            host.waits == std::vector<std::int64_t>{1} &&
            host.renderer.previous_counter == 159'630,
            "63 FPS truncates the integer interval and waits to the exact threshold");
    frame = host.frame({400'000});
    require(bool(frame) && host.renderer.previous_counter == 400'000,
            "late frame stores sampled time");
    frame = host.frame({558'729, 558'730});
    require(bool(frame) && frame.counter_reads == 2 &&
            host.renderer.previous_counter == 558'730,
            "next frame starts from the late sample, without accumulated deadline");

    host.timing.pending_logic_ticks = 12;
    require(bool(sadx::set_frame_rate_mode(2, 3, host.timing, host.renderer)) &&
            host.timing.frame_multiplier == 2 && host.timing.logic_increment == 3 &&
            host.timing.pending_logic_ticks == 0 && host.renderer.frame_interval_multiplier == 2 &&
            host.renderer.frame_interval.low == 333'333U,
            "native mode transition resets pending ticks and updates selected renderer branch");
    frame = host.frame({876'189, 876'190});
    require(bool(frame) && frame.interval == 317'460 && frame.counter_reads == 2 &&
            host.renderer.previous_counter == 876'190,
            "limiter consumes the same live mode owner after transition");
}

void explicit_unknowns() {
    NativeHost host;
    host.renderer.frame_interval_multiplier = 9;
    auto result = sadx::set_frame_interval_multiplier(2, host.renderer);
    require(result.failure == sadx::GameFrameControlFailure::unknown_frequency_high &&
            result.source_boundary == 0x7899a0 && host.renderer.frame_interval_multiplier == 9,
            "missing frequency high is explicit and preserves multiplier");
    host.renderer.counter_frequency.high = 0;
    result = sadx::set_frame_interval_multiplier(2, host.renderer);
    require(result.failure == sadx::GameFrameControlFailure::unknown_frequency_low &&
            result.source_boundary == 0x7899ab && host.renderer.frame_interval_multiplier == 9,
            "missing frequency low is explicit and preserves multiplier");
    result = sadx::set_frame_rate_mode(1, 7, host.timing, host.renderer);
    require(result.failure == sadx::GameFrameControlFailure::unknown_render_factor &&
            result.source_boundary == 0x411e72 && host.timing.frame_multiplier == 1 &&
            host.timing.logic_increment == 7 && host.timing.pending_logic_ticks == 0 &&
            !host.renderer.input_callback,
            "mode prefix survives failure before callback installation");
    host.renderer.configured_frame_interval = 1;
    result = sadx::set_frame_rate_mode(2, 1, host.timing, host.renderer);
    require(result.failure == sadx::GameFrameControlFailure::unknown_frequency_low &&
            result.source_boundary == 0x411e5e && result.nested_source_boundary == 0x7899ab,
            "nested failure retains caller and missing-fact boundaries");
    host.renderer.configured_frame_interval = 2;
    require(bool(sadx::set_frame_rate_mode(2, 1, host.timing, host.renderer)) &&
            host.renderer.frame_interval_multiplier == 9,
            "unselected renderer branch does not demand absent frequency");

    result = host.poll_input();
    require(result.failure == sadx::GameFrameControlFailure::unknown_flags &&
            result.source_boundary == 0x40f015, "unknown reset flags");
    host.input.soft_reset_flags = 2;
    result = host.poll_input();
    require(result.failure == sadx::GameFrameControlFailure::unknown_save_phase &&
            result.source_boundary == 0x40f01e && host.input.soft_reset_flags == 2,
            "unknown save phase preserves flags");
    host.input.soft_reset_flags = 0;
    result = host.poll_input();
    require(result.failure == sadx::GameFrameControlFailure::unknown_edge &&
            result.source_boundary == 0x40f037, "unknown reset edge");
    host.reset_edge = 1;
    result = host.poll_input();
    require(result.failure == sadx::GameFrameControlFailure::unknown_mask &&
            result.source_boundary == 0x40f040, "unknown reset mask");
    host.reset_edge = 0;
    require(bool(host.poll_input()) && host.input.soft_reset_flags == 0,
            "zero edge does not require absent mask or save phase");
}

void clock_failures() {
    NativeHost host;
    host.timing.frame_multiplier = 1;
    auto frame = host.frame({1});
    require(frame.failure == sadx::SpeedrunLimiterFailure::unknown_last_counter &&
            frame.source_boundary == 0x1000115b && frame.counter_reads == 1 &&
            !host.renderer.previous_counter,
            "limiter does not invent the previous counter");
    host.renderer.previous_counter = 0;
    frame = host.frame({std::nullopt});
    require(frame.failure == sadx::SpeedrunLimiterFailure::counter_unavailable &&
            frame.source_boundary == 0x10001155 && host.renderer.previous_counter == 0,
            "unavailable counter does not advance the frame");
    sadx::SpeedrunLimiterClock missing{};
    frame = sadx::run_speedrun_frame_limiter(host.renderer, host.timing, 63, missing);
    require(frame.failure == sadx::SpeedrunLimiterFailure::missing_clock &&
            frame.source_boundary == 0x1000110c, "missing native clock callback");
    frame = sadx::run_speedrun_frame_limiter(host.renderer, host.timing, 0, host.clock());
    require(frame.failure == sadx::SpeedrunLimiterFailure::divide_fault &&
            frame.source_boundary == 0x1000113d && frame.counter_reads == 0,
            "invalid cadence reports source division boundary");
    host.frequency.reset();
    frame = host.frame({std::nullopt});
    require(frame.failure == sadx::SpeedrunLimiterFailure::counter_unavailable &&
            frame.source_boundary == 0x1000119b && host.renderer.previous_counter == 0,
            "failed frequency still requires a real initialization counter");
}
} // namespace

int main() {
    try {
        host_journey();
        explicit_unknowns();
        clock_failures();
        std::cout << "{\"passed\":true,\"journeys\":[\"native_host_frame_control\","
                     "\"explicit_unknowns\",\"native_clock_failures\"]}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Native frame control: " << error.what() << '\n';
        return 1;
    }
}
