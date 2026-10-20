#pragma once
#include "hle/hle_xinput.hpp"
#include "platform/mac/hid_switch_controller.hpp"
#import <GameController/GameController.h>
namespace sadx::input::mac {
// Capture one stable extended profile as the synthetic pad's raw XInput sample.
// Apple has already applied its deadzones; this is not raw physical HID parity.
std::optional<HleXInputGamepad> xinput_sample(GCController*) noexcept;
// Inverse of the pinned SDL2 raw-input translation. The Input mod applies its mapping once.
std::optional<HleXInputGamepad> xinput_sample(const SwitchControllerSample&) noexcept;

// NOTSA: User-zero device selection for the reference app; native callers can use
// HidSwitchController's SDL-domain sample directly without the synthetic XInput adapter.
class HostControllerInput {
public:
    HostControllerInput();
    ~HostControllerInput();
    HostControllerInput(const HostControllerInput&) = delete;
    HostControllerInput& operator=(const HostControllerInput&) = delete;
    bool initialize(const std::string& native_library, std::string& error);
    std::optional<HleXInputGamepad> poll(std::string& error);
    bool rumble_supported() const noexcept;
    std::uint64_t connection_id() const noexcept;
    bool rumble(std::uint16_t low, std::uint16_t high, std::string& error);
    void shutdown();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
