#pragma once
#include <array>
#include <cstdint>
namespace sadx {
// NOTSA: numeric library configuration, excluding text resources and shared controls.
struct LibraryConfigurationDefaults {
    std::uint32_t screen_flags = 49, setup_argument = 5;
    std::array<std::int32_t, 9> buffer_counts = {
        8000, 4000, 8000, 12000, 24000, 4000, 4000, 12000, 32000};
    // The original requests UseHighPrecisionZBuffer only when this value equals one.
    std::int32_t high_precision_depth_request = 0;
    std::int32_t back_buffer_width = 640, back_buffer_height = 480;
};
// NOTSA: load-time library state, before library configuration copies and viewport setup.
struct LibraryInitialState {
    // PolyBuff_Count is an inline nine-word array; these are counts, not a pointer.
    std::array<std::int32_t, 9> buffer_counts = {
        8000, 4000, 8000, 12000, 24000, 4000, 4000, 12000, 12000};
    std::int32_t windowed = 1, back_buffer_width = 640, back_buffer_height = 480;
    float scale = 0, aspect = 0;
    LibraryConfigurationDefaults configuration{};
};
// NOTSA: maintained defaults verified against pinned file data and mapped zero fill.
LibraryInitialState library_initial_state();
} // namespace sadx
