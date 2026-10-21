#pragma once
#include <cstdint>
#include <functional>

namespace sadx::level_sounds {
// NOTSA: borrowed canonical game/sound owners. These two reset words have no
// established wider meaning; source initial storage is zero and LoadLevelSounds
// writes -6 immediately before dsStop_all.
struct State {
    const std::int16_t &stage;
    const std::int16_t &act;
    const std::uint16_t &character;
    const std::int32_t &voice_language;
    const std::uint8_t &playing_metal_sonic;
    std::uint32_t &paused;
    std::uint32_t &positional_paused;
    std::int32_t &queue_reset_word;
    std::int32_t &bank_reset_word;
};
struct Services {
    std::function<void(std::int32_t)> select_sound_bank;
    std::function<std::int32_t()> stop_all_sounds;
};
// FUNCTION: SONIC 0x4245f0 LoadCharVoices
void load_character_voices(const State &, Services &);
// FUNCTION: SONIC 0x424c80 LoadLevelSounds
std::int32_t load(State &, Services &);
} // namespace sadx::level_sounds
