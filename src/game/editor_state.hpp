#pragma once
#include <cstdint>

namespace sadx::editor {
// NOTSA: borrow the existing editor word and the distinct mission editor word.
// The SDK declares Bool, but CheckEditMode consumes signed 16-bit words.
struct View {
    const std::uint16_t &status;
    const std::uint16_t &mission_status;
};
// FUNCTION: SONIC 0x4258f0 CheckEditMode
inline std::int32_t check_mode(View state) {
    // Both loads execute, in this order; the result is not normalized to bool.
    const auto mission = static_cast<std::int16_t>(state.mission_status);
    const auto editor = static_cast<std::int16_t>(state.status);
    return static_cast<std::int32_t>(mission) | static_cast<std::int32_t>(editor);
}
} // namespace sadx::editor
