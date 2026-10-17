#include "engine/ninja_fog.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>
using namespace sadx;
namespace {
void require(bool value, const char *why) {
    if (!value)
        throw std::runtime_error(why);
}
void check(std::uint16_t control) {
    fog::State state;
    X87Context arithmetic(control);
    std::vector<std::pair<fog::RenderState, std::uint32_t>> commands;
    fog::Runtime runtime{state, arithmetic,
                         [&](auto kind, auto value) { commands.emplace_back(kind, value); },
                         1U << 20, 0};
    fog::initialize(runtime);
    require(state.values.enabled == 0 && state.values.table_supported == 1, "device setup");
    require(commands.size() == 8 &&
                commands.front() == std::make_pair(fog::RenderState::Enabled, 0U),
            "separate ordered setup commands");
    require(state.values.device.w_depth[0] == 1 && state.values.device.w_depth[127] == 248,
            "native depth table");
    require(state.values.device.reciprocal[0] == 32768, "native reciprocal table");
    commands.clear();
    fog::set_color(runtime, 0xaa123456U);
    require(state.values.device.color == 0x123456U && commands.size() == 1, "color masks alpha");
    fog::set_color(runtime, 0xbb123456U);
    require(commands.size() == 1, "unchanged color does not submit");
    fog::generate_table(runtime, -300, -3000);
    require(state.values.table.front() == 1 && state.values.table.back() == 0,
            "normalized samples");
    for (unsigned index = 1; index < 128; ++index)
        require(state.values.table[index] <= state.values.table[index - 1],
                "monotonic fog samples");
    fog::set_table(runtime);
    require(state.values.device.end_index > state.values.device.start_index, "table edge scan");
    state.profile = 1;
    state.values.level = {300, -3000, 0x123456U};
    fog::set_enabled(runtime, true);
    require(state.active == 1 && state.values.enabled == 1 && state.values.mode == 1,
            "native level enable");
    require(state.values.cached_start == -300 && state.values.cached_end == -3000,
            "negated level range cache");
    state.display_switch = 1;
    fog::set_enabled(runtime, false);
    require(state.active == 1 && state.values.enabled == 1, "display switch preserves fog state");
    state.display_switch = 0;
    fog::set_enabled(runtime, false);
    require(state.active == 0 && state.values.enabled == 0 && state.values.mode == 0,
            "native disable");
    state.profile = 0;
    fog::set_enabled(runtime, true);
    require(state.values.cached_start == -65535 && state.values.cached_end == -65535,
            "disabled profile cache");
    for (float value : state.values.table)
        require(value == 0, "disabled profile clears table");
    runtime.device_flags = 32;
    fog::initialize(runtime);
    require(state.values.table_supported == 0, "device flag disables table fog");
    bool failed = false;
    try {
        fog::generate_table(runtime, 0, std::numeric_limits<float>::infinity());
    } catch (const fog::Error &error) {
        failed = error.kind == fog::ErrorKind::NonfiniteRange;
    }
    require(failed, "nonfinite CRT range is explicit");
    runtime.render = {};
    state.values.enabled = 1;
    failed = false;
    try {
        fog::initialize(runtime);
    } catch (const fog::Error &error) {
        failed = error.kind == fog::ErrorKind::MissingDevice;
    }
    require(failed && state.values.enabled == 0, "known absent native device retains prior store");
}
} // namespace
int main() {
    try {
        for (auto control : std::array<std::uint16_t, 12>{0x7f, 0x47f, 0x87f, 0xc7f, 0x27f, 0x67f,
                                                          0xa7f, 0xe7f, 0x37f, 0x77f, 0xb7f, 0xf7f})
            check(control);
        std::cout << "Native fog: setup, tables, color, level/profile transitions and failures "
                     "across 12 controls; no assets or guest\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
