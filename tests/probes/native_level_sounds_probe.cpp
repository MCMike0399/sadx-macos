#include "game/level_sounds.hpp"
#include <algorithm>
#include <cstdint>

extern "C" int sadx_native_level_sounds(const std::uint32_t *in, std::uint32_t *out) {
    using namespace sadx::level_sounds;
    std::fill_n(out, 128, 0);
    auto stage = static_cast<std::int16_t>(in[1]);
    auto act = static_cast<std::int16_t>(in[2]);
    auto character = static_cast<std::uint16_t>(in[3]);
    auto voice_language = static_cast<std::int32_t>(in[4]);
    auto metal = static_cast<std::uint8_t>(in[5]);
    auto paused = in[6], positional_paused = in[7];
    auto queue_reset = static_cast<std::int32_t>(in[8]);
    auto bank_reset = static_cast<std::int32_t>(in[9]);
    State state{stage, act, character, voice_language, metal, paused, positional_paused,
                queue_reset, bank_reset};
    unsigned calls = 0;
    const auto event = [&](std::uint32_t kind) {
        const auto at = 32 + calls++ * 10;
        out[at] = kind;
        out[at + 1] = paused;
        out[at + 2] = positional_paused;
        out[at + 3] = static_cast<std::uint32_t>(queue_reset);
        out[at + 4] = static_cast<std::uint32_t>(bank_reset);
        out[at + 5] = character;
        out[at + 6] = static_cast<std::uint32_t>(voice_language);
        out[at + 7] = metal;
        out[at + 8] = static_cast<std::uint16_t>(stage);
        out[at + 9] = static_cast<std::uint16_t>(act);
        if (in[10] && calls == in[10]) throw 1;
        if (in[11] && calls == in[11]) {
            character = static_cast<std::uint16_t>(in[12]);
            voice_language = static_cast<std::int32_t>(in[13]);
            metal = static_cast<std::uint8_t>(in[14]);
            stage = static_cast<std::int16_t>(in[15]);
            act = static_cast<std::int16_t>(in[16]);
        }
    };
    Services services{[&](std::int32_t bank) { event(static_cast<std::uint32_t>(bank)); },
                      [&] { event(100); return static_cast<std::int32_t>(in[17]); }};
    try {
        if (in[0]) load_character_voices(state, services);
        else out[11] = static_cast<std::uint32_t>(load(state, services));
    } catch (int) { out[0] = 1; }
    out[1] = paused;
    out[2] = positional_paused;
    out[3] = static_cast<std::uint32_t>(queue_reset);
    out[4] = static_cast<std::uint32_t>(bank_reset);
    out[5] = calls;
    out[6] = character;
    out[7] = static_cast<std::uint32_t>(voice_language);
    out[8] = metal;
    out[9] = static_cast<std::uint16_t>(stage);
    out[10] = static_cast<std::uint16_t>(act);
    return 1;
}
