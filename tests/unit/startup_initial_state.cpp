#include "game/startup_initial_state.hpp"
#include "game/game_winmain.hpp"
#include <iostream>
#include <stdexcept>
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
int main() {
    try {
        const auto values = sadx::startup_initial_state();
        require(values.music_volume == -1000 && values.cursor_setting == 1 &&
                    values.text_language == sadx::TextLanguage::Japanese,
                "load-time defaults precede INI and loader settings");
        sadx::GameWinMainState state{};
        state.voice_language_at_3b0ef38 = static_cast<std::uint32_t>(values.voice_language);
        state.text_language_at_3b0f0e8 = static_cast<std::uint32_t>(values.text_language);
        state.clip_level_at_3abdcf0 = static_cast<std::uint32_t>(values.clip_level);
        state.bgm_volume_at_909f28 = static_cast<std::uint32_t>(values.music_volume);
        state.screen_at_892950 = static_cast<std::uint32_t>(values.cursor_setting);
        state.sound3d_at_3b0ef28 = static_cast<std::uint32_t>(values.sound_3d_mode);
        state.mouse_mode_at_3b0eae0 = static_cast<std::uint32_t>(values.mouse_mode);
        for (unsigned i = 0; i < 13; ++i)
            state.pad_buttons_at_389d634[i] = values.button_assignments[i];
        for (unsigned i = 0; i < 17; ++i)
            state.pad_bits_at_8929d8[i] = values.configuration_slots[i];
        // Explicit fixture resources; the native defaults do not supply these objects.
        state.media_at_3ab918c = 1;
        state.media_at_3ab9190 = 1;
        state.message_text_at_8929b0 = 0;
        state.message_caption_at_8929c4 = 0;
        std::optional<std::int32_t> interval = 99;
        std::optional<std::uint8_t> flags = 128;
        std::optional<std::uint32_t> voice_enabled = values.voice_enabled,
                                     music_enabled = values.music_enabled,
                                     voice = values.voice_volume,
                                     voice_copy = values.voice_volume_copy;
        sadx::GameWinMainRuntime runtime{
            state,
            {interval, flags, voice_enabled, voice, music_enabled, voice_copy},
            {1, 1},
            [](const sadx::WinMainProfileRead &read) -> std::optional<std::int32_t> {
                if (read.key == "buttons")
                    return 3;
                if (read.key == "pad0")
                    return 1;
                if (read.key == "pad1")
                    return 17;
                if (read.key == "pad2")
                    return 11;
                return read.fallback;
            },
            [](const sadx::WinMainProfileRead &read) -> std::optional<std::string> {
                return read.string_fallback;
            },
            [](const sadx::WinMainCall &) -> sadx::WinMainOwnerResult {
                return {{sadx::GameLogicOwnerState::completed, 0, {}}, 0};
            }};
        require(bool(sadx::run_winmain_settings(runtime)),
                "native defaults feed existing settings consumer");
        require(state.voice_language_at_3b0ef38 == 1 && state.text_language_at_3b0f0e8 == 1 &&
                    voice_enabled == 1 && music_enabled == 1 &&
                    voice == static_cast<std::uint32_t>(-300) && voice_copy == voice &&
                    state.bgm_volume_at_909f28 == static_cast<std::uint32_t>(-2100),
                "loader languages and INI volume conversions overwrite initial values");
        require(state.screen_at_892950 == 0 && state.sound3d_at_3b0ef28 == 1 &&
                    state.clip_level_at_3abdcf0 == 1 && state.mouse_mode_at_3b0eae0 == 0 &&
                    interval == 2 && flags == 128,
                "INI settings preserve separate shared control bits");
        for (unsigned i = 0; i < 13; ++i)
            require(state.pad_buttons_at_389d634[i] == (i == 1 ? 0
                                                        : i == 6
                                                            ? 1
                                                            : sadx::kUnassignedControllerButton),
                    "action-code map selects slots and ignores unsupported configuration code");
        std::cout << "Startup defaults: existing settings consumer, volumes and controller maps "
                     "passed.\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
