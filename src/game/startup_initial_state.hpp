#pragma once
#include <array>
#include <cstdint>

namespace sadx {
// LANGUAGE from SADXEnumsNew.h.
enum class TextLanguage : std::int32_t { Japanese, English, French, Spanish, German };
// NOTSA: named value for the original unassigned controller-button sentinel.
inline constexpr std::uint16_t kUnassignedControllerButton = 65535;
// NOTSA: maintained startup numeric values, before WinMain applies INI/loader settings.
// Media objects, message pointers and shared frame controls have separate owners.
struct StartupInitialState {
    std::int32_t voice_language = 0;
    TextLanguage text_language = TextLanguage::Japanese;
    std::int32_t clip_level = 0, music_volume = -1000;
    // WinMain stores zero before hiding the cursor. Wider mode meaning is unrecovered.
    std::int32_t cursor_setting = 1;
    std::int32_t sound_3d_mode = 0, mouse_mode = 0;
    std::int32_t voice_enabled = 0, music_enabled = 0, voice_volume = 0, voice_volume_copy = 0;
    // Logical slot -> physical controller-button index.
    std::array<std::uint16_t, 13> button_assignments = {
        0, 1, kUnassignedControllerButton, 3, 4, kUnassignedControllerButton, 6,
        7, 8, 5, 2, kUnassignedControllerButton, kUnassignedControllerButton};
    // Configuration action codes 1..17 -> logical slot; -1 means unsupported.
    std::array<std::int32_t, 17> configuration_slots = {
        1, 0, 8, 9, 10, 11, 12, 5, 4, 3, -1, -1, -1, -1, -1, 7, 6};
};
// NOTSA: native load-time defaults verified against pinned data and mapped zero fill.
StartupInitialState startup_initial_state();
} // namespace sadx
