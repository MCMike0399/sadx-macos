#include "runtime/session_control_defaults.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
void native_control_owners() {
    auto defaults = sadx::runtime::session_control_defaults();
    const auto &initial = defaults.renderer;
    sadx::frame::RendererState renderer{
        initial.counter_frequency,         initial.frame_interval_multiplier,
        initial.configured_frame_interval, initial.frame_interval,
        initial.previous_counter,          nullptr};
    // Explicit scripted counter ownership; no Windows clock frequency is inferred.
    renderer.counter_frequency = 10'000'000;
    sadx::frame::set_frame_rate_mode(1, 1, defaults.frame_mode, renderer);
    require(renderer.frame_interval == 500'000 && renderer.input_callback != nullptr,
            "native mode consumes the initial renderer interval setting");
    std::uint8_t edge = 1;
    sadx::frame::InputState input{edge, defaults.reset_input.soft_reset_flags,
                                  defaults.reset_input.reset_input_mask, defaults.flow.save_phase};
    input.reset_input_mask = 64;
    renderer.input_callback(input);
    require(input.soft_reset_flags == 4, "native reset input creates a pending reset");
    defaults.flow.save_phase = 1;
    renderer.input_callback(input);
    require(input.soft_reset_flags == 4, "input borrows the live save-phase owner");
    defaults.flow.save_phase = 0;
    renderer.input_callback(input);
    require(input.soft_reset_flags == 5, "completed save permits the pending reset");
}
} // namespace
int main() {
    try {
        native_control_owners();
        std::cout
            << "Session controls: native interval, callback and borrowed save phase passed.\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
