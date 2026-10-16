#include "input_initial_state_probe.hpp"
#include <cstddef>

namespace sadx::test {
std::array<std::uint8_t, 1869> input_initial_bytes(const input::InputInitialState &values) {
    std::array<std::uint8_t, 1869> bytes{};
    unsigned next = 0;
    for (const auto &row : values.edges.raw)
        for (auto value : row)
            bytes[next++] = value;
    for (auto value : values.edges.accumulated)
        bytes[next++] = value;
    bytes[next++] = values.edges.special;
    bytes[next++] = values.edges.accumulated_special;
    for (auto value : values.edges.clear_prefix)
        bytes[next++] = value;
    bytes[next++] = values.edges.cleared_neighbor;
    const auto word = [&](std::uint32_t value, unsigned size) {
        for (unsigned i = 0; i < size; ++i)
            bytes[next++] = static_cast<std::uint8_t>(value >> (8 * i));
    };
    const auto record = [&](const input::ControllerInputValues &value) {
        word(value.id, 4);
        word(value.support, 4);
        word(value.held, 4);
        word(value.not_held, 4);
        word(value.pressed, 4);
        word(value.released, 4);
        word(value.right_trigger, 2);
        word(value.left_trigger, 2);
        word(static_cast<std::uint16_t>(value.left_x), 2);
        word(static_cast<std::uint16_t>(value.left_y), 2);
        word(static_cast<std::uint16_t>(value.right_x), 2);
        word(static_cast<std::uint16_t>(value.right_y), 2);
        word(value.previous_held, 4);
    };
    record(values.controller_template);
    for (const auto &value : values.engine_raw)
        record(value);
    for (const auto &value : values.mod_raw)
        record(value);
    for (const auto &value : values.dream_pads)
        record(value);
    return bytes;
}
input::InputInitialState varied_input_initial_state() {
    auto values = input::input_initial_state();
    unsigned ordinal = 1;
    const auto byte = [&]() {
        return static_cast<std::uint8_t>(ordinal++ * 73);
    };
    for (auto &row : values.edges.raw)
        for (auto &value : row)
            value = byte();
    for (auto &value : values.edges.accumulated)
        value = byte();
    values.edges.special = byte();
    values.edges.accumulated_special = byte();
    for (auto &value : values.edges.clear_prefix)
        value = byte();
    values.edges.cleared_neighbor = byte();
    const auto record = [&](input::ControllerInputValues &value) {
        const auto index = ordinal++;
        value.id = index * 1234567;
        value.support = index * 2345678;
        value.held = index * 3456789;
        value.not_held = index * 4567890;
        value.pressed = index * 5678901;
        value.released = index * 6789012;
        value.right_trigger = static_cast<std::uint16_t>(index * 23);
        value.left_trigger = static_cast<std::uint16_t>(index * 31);
        value.left_x = -static_cast<std::int16_t>(index * 3);
        value.left_y = static_cast<std::int16_t>(index * 5);
        value.right_x = -static_cast<std::int16_t>(index * 7);
        value.right_y = static_cast<std::int16_t>(index * 11);
        value.previous_held = index * 7890123;
    };
    record(values.controller_template);
    for (auto &value : values.engine_raw)
        record(value);
    for (auto &value : values.mod_raw)
        record(value);
    for (auto &value : values.dream_pads)
        record(value);
    return values;
}
} // namespace sadx::test
extern "C" std::size_t sadx_native_input_initial_state(unsigned variant, std::uint8_t *output,
                                                       std::size_t capacity) {
    if (variant > 1 || !output || capacity < 1869)
        return 0;
    const auto values = variant == 0 ? sadx::input::input_initial_state()
                                     : sadx::test::varied_input_initial_state();
    const auto bytes = sadx::test::input_initial_bytes(values);
    for (std::size_t i = 0; i < bytes.size(); ++i)
        output[i] = bytes[i];
    return bytes.size();
}
