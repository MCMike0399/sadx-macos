#include "runtime/native_input.hpp"
#include <cassert>
#include <iostream>
using namespace sadx;
int main() {
    native::InputConfiguration config{};
    config.disable_mouse = true;
    config.keys = {{160, 69, 88, 90, 65, 83, 66, 67, 86, 13, 81, 87,
                    104, 98, 100, 102, 38, 40, 37, 39, 73, 77, 74, 76}};
    for (auto &pad : config.pads)
        pad = {false, 7849, 8689, true, false, 30};
    SseContext sse(0x1f80);
    X87Context fp(0x7f);
    std::string error;
    auto input = native::Input::create(config, sse, error);
    assert(input);
    InputModGamepadState gamepad{};
    gamepad.buttons[0] = true;
    gamepad.axes[0] = -32768;
    input->gamepad(0, gamepad);
    input->gamepad(1, gamepad);
    input->key_event(13, true);
    input->key_event(39, true);
    input->key_event(160, true);
    input->key_event(69, true);
    input->keyboard_modifiers(68);
    assert(input->poll(error));
    assert(input->update_controllers(0, PlayerInputCamera{false, {}},
                                     native::controller_constants(), fp, error));
    const auto keys = input->keyboard_keys();
    const auto before = input->snapshot(0);
    const auto analog = input->analog_snapshot(0);
    const auto held = input->held(0);
    input->cursor_state() = {-1, 123, 456, -789, 345};
    input->reset_source(); // Actual save completion occurs while Enter is held.
    assert(input->keyboard_keys() == keys);
    assert(input->switch_data() == 0);
    for (unsigned index = 0; index < 256; ++index) {
        const auto key = input->key_state(index);
        assert(key.held == 0 && key.previous == 0 && key.pressed == 0);
    }
    const auto &cursor = input->cursor_state();
    assert(cursor.y == 0 && cursor.x == 0 && cursor.magnitude == 0 &&
           cursor.cosine == 0 && cursor.sine == 0);
    const auto after = input->snapshot(0);
    assert(after.connected && after.buttons.held == before.buttons.held);
    assert(after.buttons.pressed == before.buttons.pressed);
    assert(after.buttons.left_x == before.buttons.left_x);
    assert(after.left_magnitude.bits == before.left_magnitude.bits);
    assert(after.center_camera == before.center_camera && after.half_press == before.half_press);
    assert(input->held(0) == held);
    assert(input->analog_snapshot(0).magnitude == analog.magnitude);
    input->key_event(13, false); // Normal next-frame release must not re-press Enter.
    assert(input->poll(error));
    assert(input->check_key(40) == 0 && input->key_state(40).pressed == 0);
    assert(input->key_state(79).held == 1 && input->key_state(79).pressed == 1);
    assert(input->switch_data() == 68); // Physical modifier input survived.
    assert(input->snapshot(0).left_magnitude.bits > before.left_magnitude.bits);
    assert(input->snapshot(0).center_camera && input->snapshot(0).buttons.left_x == 63);
    assert((input->snapshot(0).buttons.held & 8) == 0);
    assert(input->snapshot(0).buttons.released & 8);
    assert(input->snapshot(0).buttons.pressed == 0); // Pad owner was not reconnected.
    input->gamepad(1, std::nullopt);
    assert(input->poll(error));
    assert(input->snapshot(1).buttons.held == 4);
    input->disable_pad_reading();
    input->reset_source();
    input->reset_source();
    assert(!input->snapshot(1).connected && input->snapshot(1).buttons.held == 4);
    assert(input->poll(error));
    assert(input->update_controllers(0, PlayerInputCamera{false, {}},
                                     native::controller_constants(), fp, error));
    assert(input->published_held(0) == 0 && input->engine_snapshot(0).held == 0);
    std::cout << "Native source reset: exact logical clears, save Enter release, retained physical "
                 "keys/ramps/modifiers/pads/publication control pass\n";
}
