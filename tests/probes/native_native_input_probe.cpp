#include "runtime/native_input.hpp"
extern "C" int sadx_native_input_trace(const std::uint32_t *keys, const std::uint32_t *operations,
                                       std::size_t count, std::uint32_t *output) {
    if (!keys || !operations || !output)
        return 0;
    try {
        sadx::native::InputConfiguration config{};
        config.disable_mouse = true;
        for (std::size_t i = 0; i < config.keys.size(); ++i)
            config.keys[i] = keys[i];
        for (auto &pad : config.pads)
            pad = {false, 7849, 8689, true, false, 30};
        sadx::SseContext sse(0x1f80);
        std::string error;
        auto input = sadx::native::Input::create(config, sse, error);
        if (!input)
            return 0;
        for (std::size_t i = 0; i < count; ++i) {
            const auto *operation = operations + i * 3;
            if (operation[0] == 0)
                input->key_event(operation[1], operation[2] != 0);
            else if (operation[0] == 1)
                input->focus_lost();
            else if (operation[0] != 2)
                return 0;
            if (!input->poll(error))
                return 0;
            const auto pad = input->snapshot(0);
            const auto &b = pad.buttons;
            auto *out = output + i * 785;
            out[0] = b.id;
            out[1] = b.support;
            out[2] = b.held;
            out[3] = b.not_held;
            out[4] = b.pressed;
            out[5] = b.released;
            out[6] = b.right_trigger;
            out[7] = b.left_trigger;
            out[8] = static_cast<std::uint32_t>(static_cast<std::int32_t>(b.left_x));
            out[9] = static_cast<std::uint32_t>(static_cast<std::int32_t>(b.left_y));
            out[10] = static_cast<std::uint32_t>(static_cast<std::int32_t>(b.right_x));
            out[11] = static_cast<std::uint32_t>(static_cast<std::int32_t>(b.right_y));
            out[12] = b.previous_held;
            out[13] = pad.left_magnitude.bits;
            out[14] = pad.right_magnitude.bits;
            out[15] = sse.mxcsr();
            for (std::size_t key = 0; key < 256; ++key) {
                const auto state = input->key_state(key);
                out[16 + 3 * key] = state.held;
                out[17 + 3 * key] = state.previous;
                out[18 + 3 * key] = state.pressed;
            }
            out[784] = input->switch_data();
        }
        return 1;
    } catch (...) {
        return 0;
    }
}
extern "C" int sadx_native_input_constants(std::uint8_t *output) {
    if (!output)
        return 0;
    const auto constants = sadx::native::controller_constants();
    const auto copy = [&](std::size_t offset, const auto &value) {
        const auto *bytes = reinterpret_cast<const std::uint8_t *>(&value);
        for (std::size_t i = 0; i < sizeof value; ++i)
            output[offset + i] = bytes[i];
    };
    copy(0, constants.positive_deadzone);
    copy(4, constants.negative_deadzone);
    copy(8, constants.magnitude_scale);
    copy(12, constants.magnitude_cap);
    copy(16, constants.angle_units);
    copy(24, constants.positive_factor);
    copy(32, constants.negative_factor);
    return 1;
}
extern "C" std::uint32_t sadx_native_input_vanilla_key_index(std::uint32_t key) {
    sadx::SseContext sse(0x1f80);
    sadx::native::InputConfiguration config{};
    config.disable_mouse = true;
    std::string error;
    auto input = sadx::native::Input::create(config, sse, error);
    if (!input)
        return 0xffffffff;
    input->key_event(key, true);
    return input->keyboard_keys()[0];
}
extern "C" int sadx_native_input_soft_reset(std::uint32_t requested, std::uint32_t neighbor,
                                            std::uint32_t metal, std::uint32_t *output) {
    if (!output)
        return 0;
    sadx::native::SoftResetState state{static_cast<std::uint8_t>(requested),
                                       static_cast<std::uint8_t>(neighbor),
                                       static_cast<std::uint8_t>(metal)};
    output[0] = sadx::native::consume_soft_reset(state);
    output[1] = state.requested;
    output[2] = state.neighbor;
    output[3] = state.metal_sonic;
    return 1;
}
extern "C" int sadx_native_input_configuration(const std::int32_t *values, std::uint32_t *output) {
    if (!values || !output)
        return 0;
    sadx::native::RawInputConfiguration raw{};
    raw.disable_mouse = true;
    raw.keyboard_player = values[4];
    raw.keys.fill(values[0]);
    for (auto &pad : raw.pads)
        pad = {values[1], values[2], true, false, values[3]};
    const auto config = sadx::native::normalize_configuration(raw);
    output[0] = config.keys[0];
    output[1] = config.pads[0].deadzone_l;
    output[2] = config.pads[0].deadzone_r;
    output[3] = config.pads[0].trigger_threshold;
    output[4] = config.keyboard_player;
    return 1;
}
extern "C" int sadx_native_input_pipeline(const std::uint32_t *operations, std::size_t count,
                                          std::uint32_t *output) {
    if (!operations || !output)
        return 0;
    try {
        sadx::native::InputConfiguration config{};
        config.disable_mouse = true;
        config.keys = {{160, 69, 88,  90,  65, 83, 66, 67, 86, 13, 81, 87,
                        104, 98, 100, 102, 38, 40, 37, 39, 73, 77, 74, 76}};
        for (auto &pad : config.pads)
            pad = {false, 7849, 8689, true, false, 30};
        sadx::SseContext sse(0x1f80);
        sadx::X87Context fp(0x7f);
        std::string error;
        auto input = sadx::native::Input::create(config, sse, error);
        sadx::native::SoftResetState reset{};
        if (!input)
            return 0;
        for (std::size_t frame = 0; frame < count; ++frame) {
            const auto *operation = operations + frame * 16;
            if (operation[0] == 0)
                input->key_event(operation[1], operation[2] != 0);
            else if (operation[0] == 1)
                input->focus_lost();
            sadx::InputModGamepadState sample{};
            for (std::size_t axis = 0; axis < 6; ++axis)
                sample.axes[axis] = static_cast<std::int16_t>(operation[5 + axis]);
            for (std::size_t button = 0; button < 15; ++button)
                sample.buttons[button] = (operation[4] >> button) & 1;
            input->gamepad(0, operation[3] ? std::optional<sadx::InputModGamepadState>{sample}
                                           : std::nullopt);
            if (operation[12])
                input->enable_pad_reading();
            else
                input->disable_pad_reading();
            const bool polled = input->poll_frame({static_cast<std::int32_t>(operation[11]),
                                                   static_cast<std::int32_t>(operation[14]),
                                                   static_cast<std::int32_t>(operation[15]), -1},
                                                  reset, error);
            if (polled &&
                !input->update_controllers(static_cast<std::uint16_t>(operation[13]), {false, {}},
                                           sadx::native::controller_constants(), fp, error))
                return 0;
            auto *out = output + frame * 141;
            out[0] = polled ? 0 : 1;
            out[1] = reset.requested;
            for (std::size_t index = 0; index < 4; ++index) {
                out[2 + index] = input->held(index);
                out[6 + index] = input->pressed(index);
                out[10 + index] = input->published_held(index);
            }
            for (std::size_t index = 0; index < 8; ++index) {
                const auto value = input->engine_snapshot(index);
                auto *record = out + 14 + index * 13;
                record[0] = value.id;
                record[1] = value.support;
                record[2] = value.held;
                record[3] = value.not_held;
                record[4] = value.pressed;
                record[5] = value.released;
                record[6] = value.right_trigger | (std::uint32_t(value.left_trigger) << 16);
                record[7] = std::uint16_t(value.left_x) |
                            (std::uint32_t(std::uint16_t(value.left_y)) << 16);
                record[8] = std::uint16_t(value.right_x) |
                            (std::uint32_t(std::uint16_t(value.right_y)) << 16);
                record[9] = 0;
                record[10] = 0;
                record[11] = value.previous_held;
                record[12] = 0;
                const auto analog = input->analog_snapshot(index);
                out[118 + index * 2] = static_cast<std::uint32_t>(analog.angle);
                const auto *bytes = reinterpret_cast<const std::uint8_t *>(&analog.magnitude);
                out[119 + index * 2] = std::uint32_t(bytes[0]) | (std::uint32_t(bytes[1]) << 8) |
                                       (std::uint32_t(bytes[2]) << 16) |
                                       (std::uint32_t(bytes[3]) << 24);
            }
            const auto masks = input->button_masks();
            for (std::size_t index = 0; index < masks.size(); ++index)
                out[134 + index] = masks[index];
            out[139] = sse.mxcsr();
            out[140] = fp.exception_flags();
        }
        return 1;
    } catch (...) {
        return 0;
    }
}
