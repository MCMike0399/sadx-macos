#pragma once
#include "input/input_mod_pad.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace sadx::input::mac {
struct SwitchControllerSample {
    InputModGamepadState state;
    std::array<bool, 15> positional_buttons{}; // SDL3 values before printed-label conversion.
    std::uint32_t instance_id = 0;
    bool rumble_supported = false;
};

// NOTSA: Native Switch2 USB compatibility, via the pinned SDL3 hardware driver.
// Session-thread API. It returns the SDL2 printed-label ordering consumed by Input 2.6.1;
// the recovered Input module remains responsible for game mapping, thresholds and deadzones.
class HidSwitchController {
public:
    HidSwitchController();
    ~HidSwitchController();
    HidSwitchController(const HidSwitchController&) = delete;
    HidSwitchController& operator=(const HidSwitchController&) = delete;
    // Explicit native dependency path. No search in other applications or fallback driver.
    bool initialize(const std::string& library_path, std::string& error);
    // Disconnection/no attached Switch2Pro is nullopt with an empty error; never stale state.
    std::optional<SwitchControllerSample> poll(std::string& error);
    bool rumble(std::uint16_t low, std::uint16_t high, std::uint32_t milliseconds,
                std::string& error);
    void shutdown();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace sadx::input::mac
