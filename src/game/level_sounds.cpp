#include "game/level_sounds.hpp"
#include "game/game_cleanup.hpp"
#include <array>

namespace sadx::level_sounds {
namespace {
enum class Character : std::uint16_t { Sonic = 0, Tails = 2, Knuckles = 3, Amy = 5, Gamma = 6, Big = 7 };
// NOTSA: source switch immediates expressed as a maintained stage-to-bank table.
constexpr std::array<std::int32_t, 43> kStageBanks{
    36, 0, 56, 55, 38, 33, 52, 41, 40, 24, 37, 33, 39, 33, 33,
    27, 28, 29, 30, 32, 42, 43, 44, 33, 34, 35, 54, 54, 54,
    36, 36, 36, 36, 45, 45, 23, 51, 51, 50, 61, 61, 61, 61};
// NOTSA: select the paired Japanese/other-language bank at the source read boundary.
void select_voice(std::int32_t japanese, const State &state, Services &services) {
    services.select_sound_bank(japanese + (state.voice_language != 0 ? 1 : 0));
}
// NOTSA: this branch is shared by Sonic and unrecognized character IDs in Casinopolis.
void load_casino_default(std::int32_t bank, const State &state, Services &services) {
    services.select_sound_bank(bank);
    if (state.playing_metal_sonic != 0)
        services.select_sound_bank(62);
    else
        select_voice(59, state, services);
}
// NOTSA: separate the source Casinopolis-specific character branches from other levels.
void load_casino(const State &state, Services &services) {
    const auto character = static_cast<Character>(cleanup::get_player_number(state.character));
    switch (character) {
    case Character::Sonic:
        load_casino_default(1, state, services);
        break;
    case Character::Tails:
        services.select_sound_bank(state.playing_metal_sonic != 0 ? 62 : 1);
        select_voice(59, state, services);
        break;
    case Character::Knuckles:
        services.select_sound_bank(49);
        // The source chooses this pair before either call; do not reload language
        // after a backend call changes the borrowed owner.
        if (state.voice_language == 0) {
            services.select_sound_bank(69);
            services.select_sound_bank(57);
        } else {
            services.select_sound_bank(70);
            services.select_sound_bank(58);
        }
        break;
    case Character::Amy:
    case Character::Gamma:
    case Character::Big:
        break;
    default:
        load_casino_default(26, state, services);
        break;
    }
}
// NOTSA: character sound/voice pair used after an ordinary level bank.
void load_character(const State &state, Services &services) {
    const auto character = static_cast<Character>(cleanup::get_player_number(state.character));
    switch (character) {
    case Character::Knuckles:
        services.select_sound_bank(49);
        select_voice(69, state, services);
        break;
    case Character::Amy:
        services.select_sound_bank(46);
        select_voice(63, state, services);
        break;
    case Character::Gamma:
        services.select_sound_bank(48);
        select_voice(67, state, services);
        break;
    case Character::Big:
        services.select_sound_bank(47);
        select_voice(65, state, services);
        break;
    case Character::Tails:
        services.select_sound_bank(1);
        select_voice(71, state, services);
        break;
    default:
        services.select_sound_bank(state.playing_metal_sonic != 0 ? 62 : 1);
        select_voice(71, state, services);
        break;
    }
}
} // namespace
void load_character_voices(const State &state, Services &services) {
    const auto character = static_cast<Character>(cleanup::get_player_number(state.character));
    switch (character) {
    case Character::Knuckles:
        select_voice(69, state, services);
        break;
    case Character::Amy:
        select_voice(63, state, services);
        break;
    case Character::Gamma:
        select_voice(67, state, services);
        break;
    case Character::Big:
        select_voice(65, state, services);
        break;
    default:
        select_voice(71, state, services);
        break;
    }
}
std::int32_t load(State &state, Services &services) {
    // The source reads the HIGH byte of the act word, sign-extends it, then ORs
    // it with the unsigned stage word. Normal byte-sized acts select by stage.
    const auto act_high = static_cast<std::int8_t>(static_cast<std::uint16_t>(state.act) >> 8);
    const auto selector = static_cast<std::uint32_t>(static_cast<std::uint16_t>(state.stage)) |
                          static_cast<std::uint16_t>(static_cast<std::int16_t>(act_high));
    state.paused = 0;
    state.positional_paused = 0;
    services.select_sound_bank(selector < kStageBanks.size() ? kStageBanks[selector] : 33);
    if (selector == 9)
        load_casino(state, services);
    else if (selector >= 39 && selector <= 42)
        load_character_voices(state, services);
    else
        load_character(state, services);
    // The intervening compatibility entry returns zero without stores; its
    // unused result has no native operation (source dossier retained).
    state.queue_reset_word = -6;
    state.bank_reset_word = -6;
    return services.stop_all_sounds();
}
} // namespace sadx::level_sounds
