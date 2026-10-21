#include "engine/frame_loop_native.hpp"
#include <cstdint>

extern "C" void sadx_native_frame_counter(const std::uint32_t *in, std::uint32_t *out) {
    sadx::frame_loop::State state{static_cast<std::uint8_t>(in[0]),
                                 static_cast<std::int32_t>(in[3]),
                                 static_cast<std::int32_t>(in[4]),
                                 static_cast<std::int32_t>(in[5])};
    sadx::frame::ModeState mode{static_cast<std::int32_t>(in[2]),
                               static_cast<std::int32_t>(in[6]),
                               static_cast<std::int32_t>(in[7])};
    sadx::frame::RendererState renderer{0, 1, static_cast<std::int32_t>(in[1]), 0, 0, nullptr};
    sadx::frame_loop::complete_counter_state(state, mode, renderer);
    out[0] = state.resync;
    out[1] = state.elapsed_ticks;
    out[2] = state.loop_starting;
    out[3] = state.previous_loop_starting;
    out[4] = mode.frame_multiplier;
    out[5] = mode.logic_increment;
    out[6] = mode.pending_logic_ticks;
    out[7] = renderer.configured_frame_interval;
}
