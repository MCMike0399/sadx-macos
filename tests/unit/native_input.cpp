#include "runtime/native_input.hpp"
#include <cassert>
#include <iostream>
using namespace sadx;
static native::InputConfiguration configuration() {
    // Pinned profile config.ini: comparison order is KeyboardMouse::update_keyboard_buttons.
    native::InputConfiguration c{};
    c.disable_mouse = true;
    c.keyboard_player = 0;
    c.keys = {{160, 69, 88,  90,  65, 83, 66, 67, 86, 13, 81, 87,
               104, 98, 100, 102, 38, 40, 37, 39, 73, 77, 74, 76}};
    for (auto &pad : c.pads)
        pad = {false, 7849, 8689, true, false, 30};
    return c;
}
static void configuration_conversions() {
    native::RawInputConfiguration raw{};
    raw.disable_mouse = true;
    raw.keyboard_player = -100;
    raw.keys.fill(65536 + 88);
    for (auto &pad : raw.pads)
        pad = {65535, 65536 + 7849, true, false, 256 + 30};
    auto normalized = native::normalize_configuration(raw);
    assert(normalized.keyboard_player == 0);
    assert(normalized.keys[0] == 88);
    assert(normalized.pads[0].deadzone_l == 0);
    assert(normalized.pads[0].deadzone_r == 7849);
    assert(normalized.pads[0].trigger_threshold == 30);
    raw.keyboard_player = 100;
    raw.keys.fill(32768);
    normalized = native::normalize_configuration(raw);
    assert(normalized.keyboard_player == 7);
    assert(normalized.keys[0] == 0xffff8000U);
    assert(normalized.pads[7].allow_keyboard);
    assert(!normalized.pads[0].allow_keyboard);
}

static void controller_value_boundary() {
    ControllerInputValues values{};
    values.id = 7;
    values.support = 123;
    values.held = 12;
    values.not_held = ~12U;
    values.pressed = 4;
    values.released = 8;
    values.left_trigger = 201;
    values.right_trigger = 17;
    values.left_x = -32768;
    values.left_y = 32767;
    values.right_x = -1;
    values.right_y = 1;
    values.previous_held = 8;
    auto record = GameLogicController::from_input_values(values);
    const auto result = record.input_values().value();
    assert(result.id == 7 && result.support == 123);
    assert(result.held == 12 && result.not_held == ~12U);
    assert(result.pressed == 4 && result.released == 8);
    assert(result.left_trigger == 201 && result.right_trigger == 17);
    assert(result.left_x == -32768 && result.left_y == 32767);
    assert(result.right_x == -1 && result.right_y == 1);
    assert(result.previous_held == 8);
    record.forget(GameLogicController::held_offset, 1);
    assert(!record.input_values());
}

int main() {
    configuration_conversions();
    controller_value_boundary();
    SseContext sse(0x1f80);
    std::string error;
    auto c = configuration();
    auto bad = c;
    bad.disable_mouse = false;
    assert(!native::Input::create(bad, sse, error) && !error.empty());
    auto input = native::Input::create(c, sse, error);
    assert(input && error.empty());
    assert(input->snapshot(0).buttons.not_held ==
           0); // Initial BSS differs from post-poll complement.
    assert(input->poll(error));
    assert(input->snapshot(0).buttons.not_held == 0xffffffff);
    input->key_event(88, true);
    assert(input->poll(error));
    auto p = input->snapshot(0);
    assert(p.buttons.held == 4 && p.buttons.pressed == 4);
    assert(input->poll(error));
    assert(input->snapshot(0).buttons.held == 4 && input->snapshot(0).buttons.pressed == 0);
    input->key_event(88, false);
    assert(input->poll(error));
    assert(input->snapshot(0).buttons.released == 4);
    input->key_event(13, true);
    assert(input->poll(error));
    assert(input->snapshot(0).buttons.held == 8);
    input->key_event(13, false);
    input->key_event(81, true);
    assert(input->poll(error));
    p = input->snapshot(0);
    assert(p.buttons.left_trigger == 255 && (p.buttons.held & 131072));
    input->focus_lost();
    input->key_event(39, true);
    assert(input->poll(error));
    p = input->snapshot(0);
    assert(p.buttons.left_x == 127 && p.left_magnitude.bits != 0);
    for (int i = 0; i < 3; ++i)
        assert(input->poll(error));
    input->focus_lost();
    assert(input->poll(error));
    p = input->snapshot(0);
    assert(p.buttons.left_x == 127); // WM_KILLFOCUS preserves raw ramp.
    assert(input->poll(error));
    assert(input->poll(error));
    assert(input->poll(error));
    assert(input->snapshot(0).buttons.left_x == 0);
    input->key_event(39, true);
    input->key_event(160, true);
    assert(input->poll(error));
    assert(input->snapshot(0).buttons.left_x == 63);
    input->focus_lost();
    input->key_event(39, true);
    assert(input->poll(error));
    assert(input->snapshot(0).buttons.left_x == 63); // half-press survives focus loss.
    input->key_event(160, false);
    input->focus_lost();
    for (int i = 0; i < 4; ++i)
        assert(input->poll(error));
    InputModGamepadState gp{};
    gp.axes[0] = -32768;
    gp.axes[1] = 32767;
    gp.buttons[0] = true;
    gp.buttons[6] = true;
    input->gamepad(0, gp);
    assert(input->poll(error));
    p = input->snapshot(0);
    assert(p.connected && p.buttons.held == 12 && p.buttons.support == 0x3f0fff &&
           p.buttons.left_x < 0 && p.buttons.left_y > 0);
    input->key_event(39, true);
    assert(input->poll(error));
    assert(input->snapshot(0).buttons.left_x == 127); // Keyboard priority before physical deadzone.
    input->gamepad(0, std::nullopt);
    assert(input->poll(error));
    p = input->snapshot(0);
    assert(!p.connected && p.buttons.held == 0 && p.buttons.released == 12);
    gp.buttons[0] = false;
    gp.buttons[6] = false;
    input->gamepad(0, gp);
    assert(input->poll(error));
    assert(input->snapshot(0).buttons.held == 0);
    input->gamepad(1, gp);
    assert(input->poll(error));
    assert(input->snapshot(1).connected);
    gp.buttons[0] = true;
    input->gamepad(1, gp);
    assert(input->poll(error));
    input->gamepad(1, std::nullopt);
    assert(input->poll(error));
    assert(input->snapshot(1).buttons.held ==
           4); // Source disconnected non-keyboard slot retains last record.
    X87Context fp(0x7f);
    assert(input->update_controllers(0, PlayerInputCamera{false, {}},
                                     native::controller_constants(), fp, error));
    assert(input->held(0) & 128); // Analog direction is added by actual40f070.
    input->key_event(13, true);
    assert(input->poll(error));
    input->disable_pad_reading();
    assert(input->update_controllers(0, PlayerInputCamera{false, {}},
                                     native::controller_constants(), fp, error));
    assert(input->held(0) &
           8); // Source work owner updates even when output publication is disabled.
    assert(input->engine_snapshot(0).held == 0);
    assert(input->published_held(0) == 0);
    input->enable_pad_reading();
    assert(input->update_controllers(0, PlayerInputCamera{false, {}},
                                     native::controller_constants(), fp, error));
    assert(input->held(0) & 8);
    assert(input->engine_snapshot(0).held & 8);
    assert(input->published_held(0) & 8);
    input->key_event(115, true);
    assert(input->poll(error));
    assert(input->check_key(61) == 1 && input->key_state(61).pressed == 1);
    assert(input->poll(error));
    assert(input->check_key(61) == 1 && input->key_state(61).pressed == 0);
    input->key_event(115, false);
    assert(input->poll(error));
    assert(input->check_key(61) == 0);
    input->keyboard_modifiers(5);
    assert(input->poll(error));
    assert(input->switch_data() == 5);
    native::SoftResetState reset{2, 7, 9};
    assert(native::consume_soft_reset(reset) == 0 && reset.requested == 2 && reset.neighbor == 7 &&
           reset.metal_sonic == 9);
    reset.requested = 3;
    assert(native::consume_soft_reset(reset) == 1 && reset.requested == 0 && reset.neighbor == 0 &&
           reset.metal_sonic == 0);
    auto frame_input = native::Input::create(c, sse, error);
    native::SoftResetState frame_reset{};
    InputModGamepadState combo{};
    for (const auto button : {0, 1, 2, 3, 6})
        combo.buttons[button] = true;
    frame_input->gamepad(0, combo);
    assert(frame_input->poll_frame({4, 0, 0, 0}, frame_reset, error));
    combo.buttons[6] = false;
    frame_input->gamepad(0, combo);
    assert(frame_input->poll_frame({4, 0, 0, 0}, frame_reset, error));
    assert(frame_reset.requested == 1);
    assert(native::consume_soft_reset(frame_reset) == 1);
    combo.buttons[6] = true;
    frame_input->gamepad(0, combo);
    assert(frame_input->poll_frame({8, 0, 0, 0}, frame_reset, error));
    combo.buttons[6] = false;
    frame_input->gamepad(0, combo);
    assert(frame_input->poll_frame({8, 0, 0, 0}, frame_reset, error));
    assert(frame_reset.requested == 0);
    assert(!frame_input->poll_frame({4, 1, 1, -1}, frame_reset, error) && !error.empty());
    auto duplicate = c;
    duplicate.keys[static_cast<std::size_t>(native::KeyBinding::A)] = 13;
    auto duplicated = native::Input::create(duplicate, sse, error);
    duplicated->key_event(13, true);
    assert(duplicated->poll(error));
    assert(duplicated->snapshot(0).buttons.held == 4);
    std::cout << "Native Input: source bindings,held/edges,focus ramp/half-press,keyboard "
                 "priority,raw SDL mapping and reconnect pass\n";
}
