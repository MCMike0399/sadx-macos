#include "game/logo_startup.hpp"

namespace sadx::logo {
void load_sega_textures(State &state, StartupCalls &calls) {
    calls.initialize_heap();
    calls.initialize_tasks();
    calls.set_character_selection(0);
    calls.set_background(0, 0, 0);
    calls.load_textures("SEGALOGO_E");
    state.mode = Mode::SegaHold;
    state.frames = 0;
    calls.set_frame_rate(1, 1);
    calls.set_wait_vsync(1);
    calls.set_fade_color(0);
    calls.initialize_texture_drawing();
    // The final compatibility entry is an unconditional RET with no observable effects.
}

void initialize_sega(StartupState &state, StartupCalls &calls) {
    load_sega_textures(state.logo, calls);
    state.logo.demo_stage = 0;
    state.demo_mode = 0;
    state.demo_event = 0;
    state.demo_reset = 0;
    calls.initialize_save_data();
}
} // namespace sadx::logo
