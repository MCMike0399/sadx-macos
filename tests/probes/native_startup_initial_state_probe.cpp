#include "startup_initial_state_probe.hpp"
#include <cstddef>
namespace sadx::test {
std::array<std::uint32_t, 41> startup_initial_words(const StartupInitialState &values) {
    std::array<std::uint32_t, 41> words{};
    unsigned next = 0;
    words[next++] = static_cast<std::uint32_t>(values.voice_language);
    words[next++] = static_cast<std::uint32_t>(values.text_language);
    words[next++] = static_cast<std::uint32_t>(values.clip_level);
    words[next++] = static_cast<std::uint32_t>(values.music_volume);
    words[next++] = static_cast<std::uint32_t>(values.cursor_setting);
    words[next++] = static_cast<std::uint32_t>(values.sound_3d_mode);
    words[next++] = static_cast<std::uint32_t>(values.mouse_mode);
    words[next++] = static_cast<std::uint32_t>(values.voice_enabled);
    words[next++] = static_cast<std::uint32_t>(values.music_enabled);
    words[next++] = static_cast<std::uint32_t>(values.voice_volume);
    words[next++] = static_cast<std::uint32_t>(values.voice_volume_copy);
    for (auto value : values.button_assignments)
        words[next++] = value;
    for (auto value : values.configuration_slots)
        words[next++] = static_cast<std::uint32_t>(value);
    return words;
}
StartupInitialState varied_startup_initial_state() {
    auto values = startup_initial_state();
    values.voice_language = -1234567;
    values.text_language = static_cast<TextLanguage>(-2469134);
    values.clip_level = -3703701;
    values.music_volume = -4938268;
    values.cursor_setting = -6172835;
    values.sound_3d_mode = -7407402;
    values.mouse_mode = -8641969;
    values.voice_enabled = -9876536;
    values.music_enabled = -11111103;
    values.voice_volume = -12345670;
    values.voice_volume_copy = -13580237;
    for (unsigned i = 0; i < values.button_assignments.size(); ++i)
        values.button_assignments[i] = static_cast<std::uint16_t>(1000 + 811 * i);
    for (unsigned i = 0; i < values.configuration_slots.size(); ++i)
        values.configuration_slots[i] = -static_cast<std::int32_t>(100000 + 311 * i);
    return values;
}
} // namespace sadx::test
extern "C" std::size_t sadx_native_startup_initial_state(unsigned variant, std::uint32_t *output,
                                                         std::size_t capacity) {
    if (variant > 1 || !output || capacity < 41)
        return 0;
    const auto words = sadx::test::startup_initial_words(
        variant == 0 ? sadx::startup_initial_state() : sadx::test::varied_startup_initial_state());
    for (std::size_t i = 0; i < words.size(); ++i)
        output[i] = words[i];
    return words.size();
}
