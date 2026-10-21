#include "runtime/native_input.hpp"
extern "C" int sadx_native_input_reset_trace(const std::uint32_t *keys,
                                             const std::uint32_t *operations,
                                             std::size_t count, const std::int32_t *cursor,
                                             std::uint32_t *output) {
    using namespace sadx;
    native::InputConfiguration config{};
    config.disable_mouse = true;
    for (std::size_t index = 0; index < config.keys.size(); ++index)
        config.keys[index] = keys[index];
    for (auto &pad : config.pads)
        pad = {false, 7849, 8689, true, false, 30};
    SseContext sse(0x1f80);
    std::string error;
    auto input = native::Input::create(config, sse, error);
    if (!input)
        return 0;
    input->cursor_state() = {cursor[0], cursor[1], cursor[2], cursor[3], cursor[4]};
    for (std::size_t i = 0; i < count; ++i) {
        const auto *operation = operations + 3 * i;
        if (operation[0] == 0)
            input->key_event(operation[1], operation[2] != 0);
        else if (operation[0] == 1)
            input->focus_lost();
        else if (operation[0] == 3)
            input->reset_source();
        else if (operation[0] == 4)
            input->keyboard_modifiers(static_cast<std::uint8_t>(operation[1]));
        else if (operation[0] != 2)
            return 0;
        if (operation[0] != 3 && !input->poll(error))
            return 0;
        const auto p = input->snapshot(0);
        const auto &b = p.buttons;
        auto *out = output + i * 789;
        const std::uint32_t record[] = {
            b.id, b.support, b.held, b.not_held, b.pressed, b.released,
            b.right_trigger, b.left_trigger,
            static_cast<std::uint32_t>(static_cast<std::int32_t>(b.left_x)),
            static_cast<std::uint32_t>(static_cast<std::int32_t>(b.left_y)),
            static_cast<std::uint32_t>(static_cast<std::int32_t>(b.right_x)),
            static_cast<std::uint32_t>(static_cast<std::int32_t>(b.right_y)),
            b.previous_held, p.left_magnitude.bits, p.right_magnitude.bits};
        for (unsigned field = 0; field < 15; ++field)
            out[field] = record[field];
        for (std::size_t key = 0; key < 256; ++key) {
            const auto state = input->key_state(key);
            out[15 + 3 * key] = state.held;
            out[16 + 3 * key] = state.previous;
            out[17 + 3 * key] = state.pressed;
        }
        out[783] = input->switch_data();
        const auto &c = input->cursor_state();
        out[784] = static_cast<std::uint32_t>(c.y);
        out[785] = static_cast<std::uint32_t>(c.x);
        out[786] = static_cast<std::uint32_t>(c.magnitude);
        out[787] = static_cast<std::uint32_t>(c.cosine);
        out[788] = static_cast<std::uint32_t>(c.sine);
    }
    return 1;
}
extern "C" void sadx_native_input_reset_seeded(const std::uint32_t *values, std::uint32_t *out) {
    using namespace sadx::native;
    std::array<KeyboardKeyState, 256> keys;
    for (unsigned i = 0; i < keys.size(); ++i)
        keys[i] = {static_cast<std::uint8_t>(values[3 * i]),
                   static_cast<std::uint8_t>(values[3 * i + 1]),
                   static_cast<std::uint8_t>(values[3 * i + 2])};
    KeyboardKeyState modifier{static_cast<std::uint8_t>(values[768]),
                              static_cast<std::uint8_t>(values[769]),
                              static_cast<std::uint8_t>(values[770])};
    InputCursorState cursor{static_cast<std::int32_t>(values[771]),
                            static_cast<std::int32_t>(values[772]),
                            static_cast<std::int32_t>(values[773]),
                            static_cast<std::int32_t>(values[774]),
                            static_cast<std::int32_t>(values[775])};
    reset_input_edges(keys, modifier, cursor);
    for (unsigned i = 0; i < keys.size(); ++i) {
        out[3 * i] = keys[i].held;
        out[3 * i + 1] = keys[i].previous;
        out[3 * i + 2] = keys[i].pressed;
    }
    out[768] = modifier.held;
    out[769] = modifier.previous;
    out[770] = modifier.pressed;
    out[771] = static_cast<std::uint32_t>(cursor.y);
    out[772] = static_cast<std::uint32_t>(cursor.x);
    out[773] = static_cast<std::uint32_t>(cursor.magnitude);
    out[774] = static_cast<std::uint32_t>(cursor.cosine);
    out[775] = static_cast<std::uint32_t>(cursor.sine);
}
