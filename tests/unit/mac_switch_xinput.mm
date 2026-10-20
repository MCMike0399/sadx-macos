#include "platform/mac/mac_xinput.hpp"
#include <array>
#include <cassert>
#include <iostream>

int main() {
    using namespace sadx::input::mac;
    SwitchControllerSample sample{};
    constexpr std::array<unsigned,15> masks = {
        4096,8192,16384,32768,32,1024,16,64,128,256,512,1,2,4,8};
    for (unsigned button = 0; button < masks.size(); ++button) {
        sample.state.buttons = {};
        sample.state.buttons[button] = true;
        const auto translated = xinput_sample(sample);
        assert(translated && translated->buttons == masks[button]);
    }
    sample.state.buttons = {};
    // Every signed16 value survives the actual pinned raw-input ~Y boundary exactly.
    for (int value = -32768; value <= 32767; ++value) {
        sample.state.axes = {static_cast<std::int16_t>(value),static_cast<std::int16_t>(value),
                             static_cast<std::int16_t>(value),static_cast<std::int16_t>(value),0,32767};
        const auto translated = xinput_sample(sample);
        assert(translated && translated->left_x == value && translated->right_x == value);
        assert(-1 - static_cast<int>(translated->left_y) == value);
        assert(-1 - static_cast<int>(translated->right_y) == value);
        assert(translated->left_trigger == 0 && translated->right_trigger == 255);
    }
    sample.state.axes[4] = 32767;
    sample.state.axes[5] = 0;
    auto translated = xinput_sample(sample);
    assert(translated && translated->left_trigger == 255 && translated->right_trigger == 0);
    sample.state.axes[4] = 1;
    assert(!xinput_sample(sample));
    sample.state.axes[4] = 0;
    sample.state.axes[5] = -1;
    assert(!xinput_sample(sample));
    std::cout << "Switch2 native to reference input: 15 labels, every signed16 axis value, "
                 "exact digital triggers, unexpected analog values rejected\n";
}
