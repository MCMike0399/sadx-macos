#include "platform/mac/hid_switch_controller.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

// NOTSA: Actual native-device qualification, separate from the game's input/profile tests.
int main(int argc, char** argv) {
    if (argc < 2 || argc > 5) {
        std::cerr << "usage: controller-qualify /path/to/libSDL3.dylib [seconds] [--rumble] [capture.csv]\n";
        return 2;
    }
    const double seconds = argc > 2 ? std::strtod(argv[2], nullptr) : 5;
    if (!(seconds > 0 && seconds <= 120))
        return 2;
    const bool rumble = argc >= 4 && std::string(argv[3]) == "--rumble";
    std::ofstream capture;
    if (argc == 5) {
        capture.open(argv[4]);
        if (!capture) {
            std::cerr << "cannot create controller capture\n";
            return 1;
        }
        capture << "time_ms,instance_id,button_bits,lx,ly,rx,ry,lt,rt,rumble_supported,native_button_bits\n";
    }
    sadx::input::mac::HidSwitchController controller;
    std::string error;
    if (!controller.initialize(argv[1], error)) {
        std::cerr << error << '\n';
        return 1;
    }
    std::array<bool, 15> pressed{};
    std::array<bool, 15> released{};
    std::array<int, 6> minimum{}, maximum{};
    std::uint32_t last_id = 0;
    unsigned samples = 0, connections = 0, disconnections = 0;
    bool rumble_supported = false, rumble_accepted = false, rumble_stopped = false;
    const auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() < seconds) {
        auto sample = controller.poll(error);
        if (!error.empty()) {
            std::cerr << error << '\n';
            return 1;
        }
        if (sample) {
            if (last_id != sample->instance_id)
                ++connections;
            last_id = sample->instance_id;
            rumble_supported = sample->rumble_supported;
            for (unsigned index = 0; index < pressed.size(); ++index) {
                pressed[index] = pressed[index] || sample->state.buttons[index];
                released[index] = released[index] || (pressed[index] && !sample->state.buttons[index]);
            }
            for (unsigned index = 0; index < minimum.size(); ++index) {
                const int value = sample->state.axes[index];
                minimum[index] = samples ? std::min(minimum[index], value) : value;
                maximum[index] = samples ? std::max(maximum[index], value) : value;
            }
            ++samples;
            if (rumble && rumble_supported && !rumble_accepted) {
                rumble_accepted = controller.rumble(8000, 8000, 250, error);
                if (!rumble_accepted) {
                    std::cerr << error << '\n';
                    return 1;
                }
            }
        } else if (last_id) {
            ++disconnections;
            last_id = 0;
        }
        if (capture.is_open()) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            unsigned bits = 0;
            if (sample)
                for (unsigned index = 0; index < sample->state.buttons.size(); ++index)
                    bits |= static_cast<unsigned>(sample->state.buttons[index]) << index;
            capture << elapsed << ',' << last_id << ',' << bits;
            for (unsigned index = 0; index < minimum.size(); ++index)
                capture << ',' << (sample ? sample->state.axes[index] : 0);
            unsigned native_bits = 0;
            if (sample)
                for (unsigned index = 0; index < sample->positional_buttons.size(); ++index)
                    native_bits |= static_cast<unsigned>(sample->positional_buttons[index]) << index;
            capture << ',' << (sample && sample->rumble_supported ? 1 : 0) << ',' << native_bits << '\n';
            if (!capture) {
                std::cerr << "controller capture write failed\n";
                return 1;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (rumble_accepted)
        rumble_stopped = controller.rumble(0, 0, 0, error);
    controller.shutdown();
    bool buttons_complete = true, sticks_complete = true;
    for (unsigned index = 0; index < pressed.size(); ++index)
        if (index != 5) // Home/Guide can invoke the OS overlay; it is reported but not required.
            buttons_complete = buttons_complete && pressed[index] && released[index];
    for (unsigned index = 0; index < 4; ++index)
        sticks_complete = sticks_complete && minimum[index] < -16384 && maximum[index] > 16384;
    const bool triggers_complete = minimum[4] == 0 && minimum[5] == 0 &&
                                   maximum[4] > 16384 && maximum[5] > 16384;
    const bool reconnect_complete = connections >= 2 && disconnections >= 1;
    std::cout << "{\"backend\":\"native SDL3 shared IOHID/libusb\",\"vendor_id\":1406,"
              << "\"product_id\":8297,\"samples\":" << samples
              << ",\"connections\":" << connections << ",\"disconnections\":" << disconnections
              << ",\"rumble_supported\":" << (rumble_supported ? "true" : "false")
              << ",\"rumble_request_accepted\":" << (rumble_accepted ? "true" : "false")
              << ",\"rumble_stop_accepted\":" << (rumble_stopped ? "true" : "false")
              << ",\"buttons_observed\":[";
    for (unsigned index = 0; index < pressed.size(); ++index)
        std::cout << (index ? "," : "") << (pressed[index] ? "true" : "false");
    std::cout << "],\"axis_ranges\":[";
    for (unsigned index = 0; index < minimum.size(); ++index)
        std::cout << (index ? "," : "") << '[' << minimum[index] << ',' << maximum[index] << ']';
    std::cout << "],\"buttons_press_release_verified\":" << (buttons_complete ? "true" : "false")
              << ",\"stick_directions_verified\":" << (sticks_complete ? "true" : "false")
              << ",\"triggers_verified\":" << (triggers_complete ? "true" : "false")
              << ",\"reconnect_verified\":" << (reconnect_complete ? "true" : "false")
              << ",\"physical_gestures_verified\":"
              << (buttons_complete && sticks_complete && triggers_complete && reconnect_complete ? "true" : "false")
              << ",\"physical_rumble_feedback_verified\":false}\n";
    return samples > 0 && (!rumble_accepted || rumble_stopped) ? 0 : 1;
}
