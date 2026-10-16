#include "engine/game_frame_control.hpp"
#include "engine/speedrun.hpp"
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
using namespace sadx::frame;
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
struct NativeHost {
    ModeState timing{};
    RendererState renderer{};
    std::uint8_t reset_edge = 0;
    std::int32_t saving = 0;
    InputState input{reset_edge, 0, 0, saving};
    std::optional<std::int64_t> frequency = 10'000'000;
    std::vector<std::optional<std::int64_t>> samples;
    std::size_t cursor = 0;
    std::vector<std::int64_t> waits;
    LimiterClock clock() {
        return {[this] {
                    return frequency;
                },
                [this] {
                    if (cursor == samples.size())
                        throw std::runtime_error("counter script exhausted");
                    return samples[cursor++];
                },
                [this](std::int64_t remaining) {
                    waits.push_back(remaining);
                }};
    }
    LimiterResult frame(std::vector<std::optional<std::int64_t>> values) {
        samples = std::move(values);
        cursor = 0;
        waits.clear();
        return run_speedrun_frame_limiter(renderer, timing, sadx::speedrun::cadence_63fps, clock());
    }
    void poll() {
        require(renderer.input_callback != nullptr, "native mode installs a typed callback");
        renderer.input_callback(input);
    }
};
void host_journey() {
    NativeHost host;
    host.frequency.reset();
    auto result = host.frame({900});
    require(bool(result) && !result.frequency_available && result.counter_reads == 1 &&
                host.renderer.previous_counter == 900,
            "failed frequency initializes last from a real counter");
    host.frequency = 10'000'000;
    host.renderer.counter_frequency = *host.frequency; // supplied by the native counter owner
    host.renderer.configured_frame_interval = 1;
    set_frame_rate_mode(1, 1, host.timing, host.renderer);
    require(host.renderer.frame_interval == 166'666 && host.timing.pending_logic_ticks == 0,
            "mode setup uses integer renderer interval divided by sixty");
    host.reset_edge = 1;
    host.input.reset_input_mask = 64;
    host.poll();
    require(host.input.soft_reset_flags == 4, "native callback consumes owned input");
    host.saving = 2;
    host.poll();
    require(host.input.soft_reset_flags == 4, "active save preserves reset");
    host.saving = 0;
    host.poll();
    require(host.input.soft_reset_flags == 5, "completed save raises reset");
    result = host.frame({159'629, 159'630});
    require(bool(result) && result.interval == 158'730 && result.counter_reads == 2 &&
                host.waits == std::vector<std::int64_t>{1} &&
                host.renderer.previous_counter == 159'630,
            "63 FPS waits to its exact threshold");
    result = host.frame({400'000});
    require(bool(result) && host.renderer.previous_counter == 400'000,
            "late frame stores sampled time");
    result = host.frame({558'729, 558'730});
    require(bool(result) && result.counter_reads == 2 && host.renderer.previous_counter == 558'730,
            "next frame starts from late sample without accumulated deadline");
    host.timing.pending_logic_ticks = 12;
    set_frame_rate_mode(2, 3, host.timing, host.renderer);
    require(host.timing.frame_multiplier == 2 && host.timing.logic_increment == 3 &&
                host.timing.pending_logic_ticks == 0 && host.renderer.frame_interval == 333'333,
            "mode transition updates selected interval and resets pending ticks");
    result = host.frame({876'189, 876'190});
    require(bool(result) && result.interval == 317'460 && result.counter_reads == 2 &&
                host.renderer.previous_counter == 876'190,
            "limiter consumes the live mode owner");
}
void plain_state_values() {
    NativeHost host;
    require(host.timing.frame_multiplier == 0 && host.renderer.counter_frequency == 0 &&
                host.renderer.frame_interval == 0 && host.renderer.previous_counter == 0 &&
                host.renderer.frame_interval_multiplier == 1 &&
                host.renderer.configured_frame_interval == 3 &&
                host.renderer.input_callback == nullptr,
            "zero-filled state and file-backed one/three defaults");
    host.renderer.counter_frequency = -1;
    set_frame_interval_multiplier(-1, host.renderer);
    require(host.renderer.frame_interval == 0 && host.renderer.frame_interval_multiplier == -1,
            "signed low-word product is divided after wrapping");
    host.renderer.frame_interval_multiplier = 7;
    host.renderer.configured_frame_interval = 2;
    set_frame_rate_mode(2, -3, host.timing, host.renderer);
    require(host.renderer.frame_interval_multiplier == 7 && host.renderer.input_callback != nullptr,
            "unselected mode interval branch still installs callback");
    host.input.soft_reset_flags = 0;
    host.reset_edge = 0;
    host.input.reset_input_mask = 255;
    host.poll();
    require(host.input.soft_reset_flags == 0, "zero reset edge preserves zero flags");
}
void clock_failures() {
    NativeHost host;
    host.timing.frame_multiplier = 1;
    auto result = host.frame({std::nullopt});
    require(result.failure == LimiterFailure::CounterUnavailable && result.counter_reads == 1 &&
                result.frequency_available && host.renderer.previous_counter == 0,
            "failed counter preserves previous sample");
    result = run_speedrun_frame_limiter(host.renderer, host.timing, 63, {});
    require(result.failure == LimiterFailure::MissingClock, "missing clock owner is explicit");
    result = run_speedrun_frame_limiter(host.renderer, host.timing, 0, host.clock());
    require(result.failure == LimiterFailure::DivideFault && result.counter_reads == 0 &&
                host.renderer.counter_frequency == 10'000'000,
            "division fault follows frequency store");
    host.frequency.reset();
    result = host.frame({std::nullopt});
    require(result.failure == LimiterFailure::CounterUnavailable && !result.frequency_available,
            "failed frequency still requires an initialization counter");
    LimiterClock changing{[&] {
                              host.timing.frame_multiplier = 2;
                              return std::optional<std::int64_t>{10'000'000};
                          },
                          [] {
                              return std::optional<std::int64_t>{317'460};
                          },
                          {}};
    result = run_speedrun_frame_limiter(host.renderer, host.timing, 63, changing);
    require(bool(result) && result.interval == 317'460,
            "frequency callback mutation precedes mode read");
    unsigned reads = 0;
    changing.counter = [&] {
        host.renderer.previous_counter = 100;
        ++reads;
        return std::optional<std::int64_t>{reads == 1 ? 317'559 : 317'560};
    };
    result = run_speedrun_frame_limiter(host.renderer, host.timing, 63, changing);
    require(bool(result) && result.counter_reads == 2 && host.renderer.previous_counter == 317'560,
            "counter callback mutation precedes each previous-counter read");
}
} // namespace
int main() {
    try {
        host_journey();
        plain_state_values();
        clock_failures();
        std::cout << "{\"passed\":true,\"journeys\":[\"native_host_frame_control\",\"plain_state_"
                     "values\",\"native_clock_failures\"]}\n";
    } catch (const std::exception &error) {
        std::cerr << "Native frame control: " << error.what() << '\n';
        return 1;
    }
}
