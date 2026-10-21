#pragma once
#include "game/game_bootstrap.hpp"
#include "game/save_data.hpp"
#include <array>
#include <cstdint>
#include <functional>

namespace sadx::save {
// NOTSA: these fields borrow existing session/story owners; none is a second
// global.
struct CaptureState {
    const std::uint32_t &global_mode;
    const std::uint32_t &global_timer;
    const std::uint16_t &character;
    std::int8_t &lives;
    const std::uint8_t &playing_metal_sonic;
    const std::uint8_t &playing_super_sonic;
    const std::uint32_t &saved_stage;
    const std::array<std::uint8_t, 512> &sequence_variables;
    const std::array<std::uint8_t, 512> &cutscene_flags;
    const std::uint8_t *playable_scene_flags;
    const std::array<bootstrap::StorySequence, 8> &sequences;
    const std::array<std::uint8_t, 344> &completed_stages;
    const std::array<std::uint8_t, 60> &missions;
    const std::uint8_t &rumble_enabled;
    const std::uint32_t &market_rings;
};
// FUNCTION: SONIC 0x5065d0 FUN_005065d0
void prepare_new(bootstrap::State &, Data &, std::int16_t &rings);
// FUNCTION: SONIC 0x425b00 FUN_00425b00
void restore_title_lives(const Data &, CaptureState &);
// FUNCTION: SONIC 0x42d0d0 WriteEventFlagsToSaveFile
void store_sequence_variables(Data &, const std::array<std::uint8_t, 512> &);
// FUNCTION: SONIC 0x42d130 WriteNPCFlagsToSaveFile
void store_cutscene_flags(Data &, const std::array<std::uint8_t, 512> &);
// FUNCTION: SONIC 0x42d190 FUN_0042d190
void store_playable_scenes(Data &, const std::uint8_t *flags);
// FUNCTION: SONIC 0x42d200 WriteAdventureDataToSaveFile
void store_story_sequences(Data &, const std::array<bootstrap::StorySequence, 8> &);
// FUNCTION: SONIC 0x42d630 CreateSaveData
void capture_live_data(Data &, CaptureState &);
// FUNCTION: SONIC 0x5065e0 FUN_005065e0
std::int32_t request_write(const std::int32_t &saving_disabled, std::uint8_t &save_ready,
                           const std::function<void()> &capture_live);
} // namespace sadx::save
