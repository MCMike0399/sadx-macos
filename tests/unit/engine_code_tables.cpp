#include "runtime/engine_code_tables.hpp"
#include "engine/ninja_keyboard.hpp"
#include "engine/ninja_render.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
void renderer_uses_adjacent_comparisons() {
    const auto native = sadx::runtime::engine_code_tables();
    sadx::NinjaBlendTables blend{};
    for (std::size_t i = 0; i < blend.states.size(); ++i)
        blend.states[i] = static_cast<std::uint32_t>(native.blend_targets[i]);
    for (std::size_t i = 0; i < blend.values.size(); ++i)
        blend.values[i] = static_cast<std::uint32_t>(native.blend_modes[i]);
    std::array<std::uint32_t, 8> depth{};
    for (std::size_t i = 0; i < depth.size(); ++i)
        depth[i] = static_cast<std::uint32_t>(native.depth_comparisons[i]);
    blend.following_words_at_88ad9c =
        std::array<std::uint32_t, 4>{depth[0], depth[1], depth[2], depth[3]};
    sadx::NinjaRenderState renderer{};
    renderer.device_at_3d128b0 =
        std::make_shared<sadx::NinjaRenderDevice>(sadx::NinjaRenderDevice{1});
    renderer.blend_tables = &blend;
    std::vector<std::uint32_t> values;
    renderer.set_render_state = [&](const sadx::NinjaRenderStateCommand &command) {
        values.push_back(command.value);
        return sadx::NinjaRenderCallResult{true, 0};
    };
    for (std::uint32_t index = 12; index < 16; ++index)
        require(bool(sadx::ninja_set_blend_77ec60(0, index, renderer)),
                "unchecked blend tail is available");
    for (std::uint32_t index = 0; index < 4; ++index)
        require(bool(sadx::ninja_set_depth_comparison(index, &depth, renderer)),
                "depth comparison available");
    require(values.size() == 8 &&
                std::equal(values.begin(), values.begin() + 4, values.begin() + 4),
            "blend tail and depth comparisons share their source values");
}
void keyboard_retains_held_order() {
    const auto native = sadx::runtime::engine_code_tables();
    sadx::NinjaKeyboardTable table{};
    for (std::size_t i = 0; i < native.keyboard.size(); ++i) {
        table.kind[i] = static_cast<std::uint16_t>(native.keyboard[i].kind);
        table.code[i] = native.keyboard[i].code;
    }
    sadx::NinjaKeyboardState state{};
    state.device_known = true;
    state.count_at_3d0881c = 1;
    state.records_at_3d08818 = 0x3ce6e38;
    auto &record = state.records_at_3ce6e38[0];
    record.keys = {4, 0, 0, 0, 0, 0}; // A was held before Escape was pressed.
    state.device_at_3d08820 = std::make_shared<sadx::NinjaKeyboardDevice>();
    state.device_at_3d08820->get_device_state = [] {
        std::array<std::uint8_t, 256> keys{};
        keys[1] = keys[29] = keys[30] = 128; // Escape, left Control, A
        return std::pair{0, keys};
    };
    const auto result = sadx::ninja_keyboard_poll_791bd0(state, table);
    require(bool(result) && record.modifiers == 1 && record.keys[0] == 4 && record.keys[1] == 41 &&
                record.keys[2] == 0,
            "native translations preserve held order and distinguish modifiers from keys");
}
} // namespace
int main() {
    try {
        renderer_uses_adjacent_comparisons();
        keyboard_retains_held_order();
        std::cout
            << "Engine code tables: shared blend/depth tail and keyboard held order passed.\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
