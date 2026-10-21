#include "engine/ui_scale_profile.hpp"
#include <cassert>
#include <stdexcept>
int main() {
    float horizontal = 1, vertical = 1;
    sadx::ui_scale::State state{horizontal, vertical};
    assert(!sadx::ui_scale::scaling_enabled(state));
    unsigned calls = 0;
    sadx::ui_scale::run_identity_background(state, 640, 480, [&] {
        ++calls;
        assert(state.enabled && state.entries.size() == 1);
        bool rejected = false;
        try {
            sadx::ui_scale::require_message_forwarding(state);
        } catch (const std::runtime_error &) {
            rejected = true;
        }
        assert(rejected);
        sadx::ui_scale::run_identity_background(state, 640, 480, [&] {
            ++calls;
            assert(state.entries.size() == 2);
        });
        assert(state.entries.size() == 1 && state.enabled);
    });
    assert(calls == 2 && state.entries.empty() && !state.enabled);
    sadx::ui_scale::require_message_forwarding(state);
    bool threw = false;
    try {
        sadx::ui_scale::run_identity_background(state, 640, 480,
                                                [&] { throw std::runtime_error("body"); });
    } catch (const std::runtime_error &) {
        threw = true;
    }
    assert(threw && state.entries.size() == 1 && state.enabled);
    state.background_fill = sadx::ui_scale::FillMode::Stretch;
    assert(!sadx::ui_scale::scaling_enabled(state));
    state.entries.back().background = false;
    assert(sadx::ui_scale::scaling_enabled(state));
}
