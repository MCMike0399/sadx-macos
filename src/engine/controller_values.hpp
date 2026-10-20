#pragma once
#include <cstdint>

namespace sadx {
// NOTSA: the input values of PDS_PERIPHERAL, independent of its resource pointers.
struct ControllerInputValues {
    std::uint32_t id = 0;
    std::uint32_t support = 0;
    std::uint32_t held = 0;
    std::uint32_t not_held = 0;
    std::uint32_t pressed = 0;
    std::uint32_t released = 0;
    std::uint16_t right_trigger = 0, left_trigger = 0;
    std::int16_t left_x = 0, left_y = 0, right_x = 0, right_y = 0;
    std::uint32_t previous_held = 0;
};
} // namespace sadx
