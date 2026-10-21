#include "guest/caption_frame_reference.hpp"
#include <cassert>

int main() {
    using namespace sadx;
    menu_caption::State state;
    reference::CaptionFrameReference adapter(state);
    adapter.observe_timer(7);
    adapter.observe_buffer(4096);
    std::weak_ptr<menu_caption::Buffer> released = state.buffer;
    assert(adapter.buffer_word() == 4096);
    // The adapter observes the game owner itself; it keeps no shadow timer or strong buffer owner.
    state.timer = 1;
    assert(adapter.timer_word() == 1);
    unsigned calls = 0;
    const auto result = adapter.complete_frame([&](const GameFrameOwnerCall &call) {
        ++calls;
        assert(state.timer == 0);
        if (calls == 1) {
            assert(call.callee == 0x40e2c0);
            adapter.observe_buffer(8192);
            assert(released.expired());
        } else {
            assert(call.callee == 0x780fb0 && call.arguments[0] == 8192);
        }
        return GameLogicOwnerResult{GameLogicOwnerState::completed, call.callee, {}};
    });
    assert(result && calls == 2 && !state.buffer);
    assert(adapter.buffer_word() == 0);
    adapter.observe_buffer(12288);
    released = state.buffer;
    adapter.observe_buffer(0);
    assert(released.expired());
}
