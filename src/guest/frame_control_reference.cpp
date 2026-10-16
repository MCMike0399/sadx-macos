#include "guest/frame_control_reference.hpp"
#include "engine/game_frame_control_internal.hpp"
#include <cstring>
#include <stdexcept>

namespace sadx {
namespace {
struct ReferenceStop {};
struct Binding {
    FrameRateModeState *timing = nullptr;
    const FrameRateModeState *timing_input = nullptr;
    GameFrameRendererState *renderer = nullptr;
    GameFrameInputState *input = nullptr;
    GameFrameControlResult control{};
    SpeedrunLimiterResult limiter{};
};
thread_local Binding *active = nullptr;
Binding &binding() {
    if (!active)
        throw std::logic_error("frame-control reference binding is absent");
    return *active;
}
struct BindScope {
    Binding *previous;
    explicit BindScope(Binding &current) : previous(active) { active = &current; }
    ~BindScope() { active = previous; }
};
[[noreturn]] void stop(GameFrameControlFailure failure, std::uint32_t site) {
    binding().control = {failure, site, 0, false};
    throw ReferenceStop{};
}
[[noreturn]] void stop_limiter(SpeedrunLimiterFailure failure, std::uint32_t site) {
    binding().limiter.failure = failure;
    binding().limiter.source_boundary = site;
    throw ReferenceStop{};
}
std::int64_t signed_counter(std::uint32_t low, std::uint32_t high) {
    const auto bits = (std::uint64_t(high) << 32) | low;
    std::int64_t result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}
void store_counter(CounterWords &destination, std::int64_t value) {
    destination.low = static_cast<std::uint32_t>(std::uint64_t(value));
    destination.high = static_cast<std::uint32_t>(std::uint64_t(value) >> 32);
}
SpeedrunLimiterResult translate(const frame::LimiterResult &result) {
    SpeedrunLimiterResult out{};
    out.interval = result.interval;
    out.counter_reads = result.counter_reads;
    out.frequency_available = result.frequency_available;
    out.completed = result.completed;
    switch (result.failure) {
    case frame::LimiterFailure::None:
        break;
    case frame::LimiterFailure::MissingClock:
        out.failure = SpeedrunLimiterFailure::missing_clock;
        out.source_boundary = 0x1000110c;
        break;
    case frame::LimiterFailure::CounterUnavailable:
        out.failure = SpeedrunLimiterFailure::counter_unavailable;
        out.source_boundary = result.frequency_available ? 0x10001155 : 0x1000119b;
        break;
    case frame::LimiterFailure::DivideFault:
        out.failure = SpeedrunLimiterFailure::divide_fault;
        out.source_boundary = 0x1000113d;
        break;
    }
    return out;
}
} // namespace
GameFrameControlResult set_frame_interval_multiplier(std::int32_t value,
                                                     GameFrameRendererState &renderer) {
    Binding current{nullptr, nullptr, &renderer, nullptr, {}, {}};
    BindScope scope(current);
    frame::RendererState view{};
    try {
        frame::set_frame_interval_multiplier(value, view);
        current.control.completed = true;
    } catch (const ReferenceStop &) {
    }
    return current.control;
}
GameFrameControlResult set_frame_rate_mode(std::int32_t render_ratio, std::int32_t logic_ratio,
                                           FrameRateModeState &timing,
                                           GameFrameRendererState &renderer) {
    Binding current{&timing, &timing, &renderer, nullptr, {}, {}};
    BindScope scope(current);
    frame::ModeState mode_view{};
    frame::RendererState renderer_view{};
    try {
        frame::set_frame_rate_mode(render_ratio, logic_ratio, mode_view, renderer_view);
        current.control.completed = true;
    } catch (const ReferenceStop &) {
    }
    return current.control;
}
GameFrameControlResult update_soft_reset_flags(GameFrameInputState &input) {
    Binding current{nullptr, nullptr, nullptr, &input, {}, {}};
    BindScope scope(current);
    std::uint8_t edge = 0;
    std::int32_t save = 0;
    frame::InputState view{edge, 0, 0, save};
    try {
        frame::update_soft_reset_flags(view);
        current.control.completed = true;
    } catch (const ReferenceStop &) {
    }
    return current.control;
}
GameFrameControlResult run_frame_input(GameFrameInputState &input) {
    Binding current{nullptr, nullptr, nullptr, &input, {}, {}};
    BindScope scope(current);
    std::uint8_t edge = 0;
    std::int32_t save = 0;
    frame::InputState view{edge, 0, 0, save};
    try {
        frame::run_frame_input(view);
        current.control.completed = true;
    } catch (const ReferenceStop &) {
    }
    return current.control;
}
SpeedrunLimiterResult run_speedrun_frame_limiter(GameFrameRendererState &renderer,
                                                 const FrameRateModeState &timing,
                                                 std::int32_t cadence,
                                                 const SpeedrunLimiterClock &clock) {
    // The view is only transport: getters validate the authoritative reference facts lazily.
    Binding current{nullptr, &timing, &renderer, nullptr, {}, {}};
    BindScope scope(current);
    frame::RendererState renderer_view{};
    frame::ModeState mode_view{};
    try {
        return translate(
            frame::run_speedrun_frame_limiter(renderer_view, mode_view, cadence, clock));
    } catch (const ReferenceStop &) {
        return current.limiter;
    }
}
namespace frame {
std::int64_t counter_frequency(const RendererState &) {
    const auto &value = binding().renderer->counter_frequency;
    if (!value.high)
        stop(GameFrameControlFailure::unknown_frequency_high, 0x7899a0);
    if (!value.low)
        stop(GameFrameControlFailure::unknown_frequency_low, 0x7899ab);
    return signed_counter(*value.low, *value.high);
}
void store_counter_frequency(RendererState &view, std::int64_t value) {
    store_counter(binding().renderer->counter_frequency, value);
    view.counter_frequency = value;
}
void store_interval_multiplier(RendererState &view, std::int32_t value) {
    binding().renderer->frame_interval_multiplier = value;
    view.frame_interval_multiplier = value;
}
void store_frame_interval(RendererState &view, std::int64_t value) {
    store_counter(binding().renderer->frame_interval, value);
    view.frame_interval = value;
}
std::int32_t configured_frame_interval(const RendererState &, ModeInterval mode) {
    const auto &value = binding().renderer->configured_frame_interval;
    if (!value)
        stop(GameFrameControlFailure::unknown_render_factor,
             mode == ModeInterval::Normal ? 0x411e72 : 0x411e53);
    return *value;
}
void apply_mode_interval(RendererState &view, std::int32_t value, ModeInterval mode) {
    try {
        frame::set_frame_interval_multiplier(value, view);
    } catch (const ReferenceStop &) {
        auto &result = binding().control;
        result.nested_source_boundary = result.source_boundary;
        result.source_boundary = mode == ModeInterval::Normal ? 0x411e79 : 0x411e5e;
        throw;
    }
}
void store_frame_multiplier(ModeState &view, std::int32_t value) {
    binding().timing->frame_multiplier = value;
    view.frame_multiplier = value;
}
void store_logic_increment(ModeState &view, std::int32_t value) {
    binding().timing->logic_increment = value;
    view.logic_increment = value;
}
void clear_pending_logic_ticks(ModeState &view) {
    binding().timing->pending_logic_ticks = 0;
    view.pending_logic_ticks = 0;
}
void store_input_callback(RendererState &view, InputCallback callback) {
    if (callback != &frame::run_frame_input)
        throw std::logic_error("unknown frame input callback identity");
    binding().renderer->input_callback = &sadx::run_frame_input;
    view.input_callback = callback;
}
std::uint8_t soft_reset_flags(const InputState &) {
    const auto &value = binding().input->soft_reset_flags;
    if (!value)
        stop(GameFrameControlFailure::unknown_flags, 0x40f015);
    return *value;
}
void store_soft_reset_flags(InputState &view, std::uint8_t value) {
    binding().input->soft_reset_flags = value;
    view.soft_reset_flags = value;
}
std::int32_t save_phase(const InputState &) {
    const auto &value = binding().input->save_phase;
    if (!value)
        stop(GameFrameControlFailure::unknown_save_phase, 0x40f01e);
    return *value;
}
std::uint8_t reset_key_edge(const InputState &) {
    const auto &value = binding().input->reset_key_edge;
    if (!value)
        stop(GameFrameControlFailure::unknown_edge, 0x40f037);
    return *value;
}
std::uint8_t reset_input_mask(const InputState &) {
    const auto &value = binding().input->reset_input_mask;
    if (!value)
        stop(GameFrameControlFailure::unknown_mask, 0x40f040);
    return *value;
}
std::int32_t frame_multiplier(const ModeState &) {
    const auto &value = binding().timing_input->frame_multiplier;
    if (!value)
        stop_limiter(SpeedrunLimiterFailure::unknown_frame_multiplier, 0x1000111c);
    return *value;
}
std::int64_t previous_counter(const RendererState &) {
    const auto &value = binding().renderer->previous_counter;
    if (!value)
        stop_limiter(SpeedrunLimiterFailure::unknown_last_counter, 0x1000115b);
    return *value;
}
void store_previous_counter(RendererState &view, std::int64_t value) {
    binding().renderer->previous_counter = value;
    view.previous_counter = value;
}
void report_limiter_progress(const LimiterResult &progress) {
    binding().limiter = translate(progress);
}
} // namespace frame
} // namespace sadx
