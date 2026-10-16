#include "input/input_mod_constants.hpp"
#include <iostream>
#include <stdexcept>
int main() {
    auto constants = sadx::input_mod_261_constants();
    sadx::SseContext fp(0x1f80);
    sadx::InputModAxesOutput left{}, right{};
    sadx::normalize_input_mod_261_keyboard({16384, 0}, {0, 0}, constants.axes, fp, left, right);
    if (left.x != 127 || left.y != 0 || right.x != 0 || right.y != 0)
        throw std::runtime_error(
            "native coefficients preserve keyboard direction and zero-stick guard");
    sadx::InputModDreamPad pad{};
    for (unsigned word = 0; word < 13; ++word)
        pad.record.write_u32(word * 4, 0);
    pad.connected = true;
    pad.settings.deadzone_r = 8689;
    pad.settings.radial_l = true;
    pad.settings.trigger_threshold = 30;
    sadx::InputModGamepadState gamepad{};
    gamepad.axes = {16384, 0, 0, 0, 32767, 0};
    gamepad.buttons[8] = true;
    const auto result = sadx::input_mod_poll(pad, {}, gamepad, false, constants, fp);
    if (!result || !pad.half_press || pad.record.read_u16(28) != 32 ||
        pad.record.read_u16(26) != 255)
        throw std::runtime_error(
            "controller128, half-press0.5 and trigger255 coefficients compose");
    constants.axes.controller_scale.bits = 0;
    if (!sadx::input_mod_261_constants().axes.controller_scale.bits)
        throw std::runtime_error("callers own independent coefficient values");
    std::cout
        << "Input constants: keyboard, controller, trigger and half-press consumers passed.\n";
}
