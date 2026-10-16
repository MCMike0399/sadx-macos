#include "sound_initial_state_probe.hpp"
#include <cstddef>
#include <type_traits>

namespace sadx::test {
std::array<std::uint32_t, 329> sound_initial_words(const SoundInitialState &values) {
    std::array<std::uint32_t, 329> words{};
    unsigned next = 0;
    words[next++] = static_cast<std::uint32_t>(values.paused);
    words[next++] = static_cast<std::uint32_t>(values.positional_paused);
    words[next++] = static_cast<std::uint32_t>(values.queue_count);
    words[next++] = static_cast<std::uint32_t>(values.frame_counter);
    words[next++] = static_cast<std::uint32_t>(values.stream_initialized);
    for (auto byte : values.overlap)
        words[next++] = byte;
    for (const auto &record : values.records) {
        words[next++] = static_cast<std::uint32_t>(record.timer);
        words[next++] = static_cast<std::uint32_t>(record.flags);
        words[next++] = static_cast<std::uint32_t>(record.tone);
        words[next++] = static_cast<std::uint32_t>(record.pan_angle);
        words[next++] = static_cast<std::uint32_t>(record.volume);
        words[next++] = static_cast<std::uint32_t>(record.pitch);
        words[next++] = static_cast<std::uint32_t>(record.current_volume);
    }
    return words;
}
SoundInitialState varied_sound_initial_state() {
    auto values = sound_initial_state();
    std::uint32_t ordinal = 0;
    const auto vary = [&](auto &value) {
        using T = std::decay_t<decltype(value)>;
        const auto index = ++ordinal;
        if constexpr (std::is_same_v<T, std::int32_t>)
            value = -static_cast<std::int32_t>(index * 1234567U);
        else
            value = static_cast<T>(index * 1234567U);
    };
    vary(values.paused);
    vary(values.positional_paused);
    vary(values.queue_count);
    vary(values.frame_counter);
    vary(values.stream_initialized);
    for (auto &byte : values.overlap)
        vary(byte);
    for (auto &record : values.records) {
        vary(record.timer);
        vary(record.flags);
        vary(record.tone);
        vary(record.pan_angle);
        vary(record.volume);
        vary(record.pitch);
        vary(record.current_volume);
    }
    return values;
}
} // namespace sadx::test

extern "C" std::size_t sadx_native_sound_initial_state(unsigned variant, std::uint32_t *output,
                                                       std::size_t capacity) {
    if (variant > 1 || !output || capacity < 329)
        return 0;
    const auto values =
        variant == 0 ? sadx::sound_initial_state() : sadx::test::varied_sound_initial_state();
    const auto words = sadx::test::sound_initial_words(values);
    for (std::size_t i = 0; i < words.size(); ++i)
        output[i] = words[i];
    return words.size();
}
