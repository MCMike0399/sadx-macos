#include "engine/ninja_fog.hpp"
#include <cassert>
#include <vector>
using namespace sadx;
int main() {
    fog::State state;
    X87Context arithmetic(127);
    std::vector<std::pair<fog::RenderState, unsigned>> calls;
    fog::Runtime runtime{state, arithmetic,
        [&](fog::RenderState setting, unsigned value) { calls.emplace_back(setting, value); },
        1048576, 0};
    // These capabilities are explicit test inputs, not inferred native defaults.
    fog::initialize(runtime);
    calls.clear();
    state.active = 1;
    state.values.enabled = 1;
    state.display_switch = 0;
    std::int32_t pending_logic_ticks = 9;
    fog::set_enabled(runtime, false, pending_logic_ticks);
    assert(state.active == 1 && state.values.enabled == 1 && calls.empty());
    pending_logic_ticks = 0;
    state.display_switch = 17; // A stale compatibility copy must not suppress the real caller.
    fog::set_enabled(runtime, false, pending_logic_ticks);
    assert(state.active == 0 && state.values.enabled == 0);
    assert(calls.size() == 1 && calls[0].first == fog::RenderState::Enabled && calls[0].second == 0);
    assert(state.display_switch == 17 && pending_logic_ticks == 0);
    calls.clear();
    state.values.enabled = 1;
    state.active = 1;
    fog::set_enabled(runtime, false); // Existing API still reads its original compatibility owner.
    assert(state.active == 1 && state.values.enabled == 1 && calls.empty());
}
