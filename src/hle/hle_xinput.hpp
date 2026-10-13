#pragma once
#include <cstdint>
#include <optional>
namespace sadx {
class HleProcess;
// One synthetic wired XInput gamepad at user 0. Axes are raw XInput values (Y up),
// not SDL axes; the original SDL DLL performs the conversion and deadzone processing.
struct HleXInputGamepad {
    std::uint16_t buttons=0;
    std::uint8_t left_trigger=0,right_trigger=0;
    std::int16_t left_x=0,left_y=0,right_x=0,right_y=0;
    bool operator==(const HleXInputGamepad& other)const noexcept;
};
struct HleXInputState {
    std::optional<HleXInputGamepad> pad;
    std::uint32_t packet=0;
    bool enabled=true;
    void sample(std::optional<HleXInputGamepad>);
};
void register_hle_xinput(HleProcess&);
} // namespace sadx
