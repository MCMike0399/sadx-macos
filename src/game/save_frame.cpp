#include "game/save_frame.hpp"
namespace sadx::save {
void before_frame_completion(FrameStateView state, FrameCalls &calls) {
    if (state.ready != 0) {
        state.count = static_cast<std::uint8_t>(state.count + 1U);
        state.now_saving = 1;
        state.exec = 1;
    }
    const auto phase = state.now_saving;
    if (phase == 1 || phase == -1) {
        calls.display_saving_message(state.message);
    } else if (state.pending_message != 0) {
        calls.close_saving_message(state.message);
        state.pending_message = 0;
    }
}
void after_frame_completion(FrameStateView state, FrameCalls &calls) {
    if (state.exec != 0) {
        state.exec = 0;
        calls.write_save_file();
    }
    if (state.now_saving == 0) {
        if (state.pending_message != 0) {
            calls.close_saving_message(state.message);
            state.pending_message = 0;
        }
        if (state.pause_state == 2)
            state.pause_state = 1;
    }
}
} // namespace sadx::save
