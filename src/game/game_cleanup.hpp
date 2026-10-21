#pragma once
#include "game/save_data.hpp"
#include "game/game_bootstrap.hpp"
#include "game/sound_initial_state.hpp"
#include "engine/task.hpp"
#include "engine/sprite_immediate.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace sadx::cleanup {
// The wider meaning of reset_status and the two temporary buffers remains unrecovered.
// They are live session owners, not copies that can diverge during service callbacks.
struct State {
    std::uint32_t &reset_status;
    std::uint16_t &editor_status;
    void *&temporary_buffer;
    void *&secondary_buffer;
    std::uint32_t &random_seed;
};
struct Services {
    std::function<void()> resume_voice_volume;
    std::function<void(std::uint32_t)> set_depth_compare_mode;
    std::function<void(std::uint32_t)> stop_vibration;
    std::function<void()> set_blank_queue_mode;
    std::function<void()> reset_song_state;
    std::function<void()> reset_current_voice;
    std::function<void()> stop_all_sounds;
    std::function<void(std::int32_t)> stop_pcm;
    std::function<void(std::uint32_t)> close_media;
    std::function<void()> setup_screen;
    std::function<void(void *)> display_message;
    std::function<void()> initialize_heap;
    std::function<void()> initialize_tasks;
    std::function<void(std::uint32_t)> run_level_destructor;
    std::function<void()> reset_loop_state;
    std::function<void(void *)> release_buffer;
    std::function<void()> enable_fog;
};
struct AudioQueueView {
    std::array<SoundRecordInitialValues, 36> &records;
    std::array<TaskHandle, 36> &positional_tasks;
};
// FUNCTION: SONIC 0x40cd10 voicevolresume
void resume_voice_volume(std::int32_t &current, const std::int32_t &saved);
// FUNCTION: SONIC 0x424460 dsStop_all
std::int32_t stop_all_sounds(AudioQueueView &, const std::function<void(std::int32_t)> &stop_pcm);
struct LoadSaveState {
    std::uint32_t &global_timer;
    std::uint16_t &character;
    std::int8_t &lives;
    const std::uint8_t &playing_metal_sonic;
    std::uint32_t &saved_stage;
    std::array<std::uint8_t, 512> &sequence_variables;
    std::array<std::uint8_t, 512> &cutscene_flags;
    std::uint8_t *playable_scene_flags; // borrowed native slice, exactly eight elements
    std::array<bootstrap::StorySequence, 8> &sequences;
    std::array<std::uint8_t, 344> &completed_stages;
    std::array<std::uint8_t, 60> &missions;
    std::uint8_t &rumble_enabled;
    std::uint32_t &market_rings;
};
// FUNCTION: SONIC 0x4144d0 SetPlayerNumber
void set_player_number(std::uint16_t &current, std::uint16_t number);
// FUNCTION: SONIC 0x4144e0 GetPlayerNumber
std::uint16_t get_player_number(const std::uint16_t &current);
// FUNCTION: SONIC 0x42d250 FUN_0042d250
void load_sequence_variables(const save::Data &, std::array<std::uint8_t, 512> &);
// FUNCTION: SONIC 0x42d2a0 FUN_0042d2a0
void load_cutscene_flags(const save::Data &, std::array<std::uint8_t, 512> &);
// FUNCTION: SONIC 0x42d2f0 FUN_0042d2f0
void load_playable_scenes(const save::Data &, std::uint8_t *flags);
// FUNCTION: SONIC 0x42d360 FUN_0042d360
void load_story_sequences(const save::Data &, std::array<bootstrap::StorySequence, 8> &);
// FUNCTION: SONIC 0x42d770 LoadSave
void load_save(const save::Data &, LoadSaveState &);
struct PlacementEntry;
// NOTSA: native callbacks borrow session context instead of reading guest globals.
using LevelDestructor = void (*)(void *context);
struct LevelState {
    std::uint32_t *&set_entry_count;
    PlacementEntry *&set_entries;
    std::array<void *, 6> &set_files;
    std::array<void *, 6> &camera_files;
    LevelDestructor &destructor;
    void *&destructor_context;
    std::string &debug_message;
};
struct LoopState {
    std::uint32_t &reset_work;
    std::uint32_t &loop_count;
    const void *&list_data;
};
// STUB: SONIC 0x592370 LevelDestructor_MissionMode awaiting mission cleanup
void mission_level_destructor(void *context);
// FUNCTION: SONIC 0x422440 ReleaseSetFile
void release_set_files(LevelState &, const std::function<void(void *)> &release_buffer);
// FUNCTION: SONIC 0x4224b0 ReleaseCamFile
void release_camera_files(LevelState &, const std::function<void(void *)> &release_buffer);
// FUNCTION: SONIC 0x4228f0 RunLevelDestructor
void run_level_destructor(std::uint32_t mode, LevelState &,
                           const std::function<void(void *)> &release_buffer);
// FUNCTION: SONIC 0x421fb0 FUN_00421fb0
void reset_loop_state(LoopState &, const std::function<void()> &clear_list);
struct CharacterTextureLists {
    std::vector<sprite::TextureList *> lists;
};
struct TextureState {
    sprite::TextureList *&common;
    std::array<sprite::TextureList *, 32> &regular;
    const std::array<const CharacterTextureLists *, 9> &characters;
    std::array<std::int8_t, 9> &loaded;
};
using ReleaseTextureList = std::function<void(sprite::TextureList &)>;
// FUNCTION: SONIC 0x420f40 FreeRegObjTexlists
void release_regular_object_textures(TextureState &, const ReleaseTextureList &);
// FUNCTION: SONIC 0x420ef0 FreeCharacterTexlists
void release_character_textures(TextureState &, const ReleaseTextureList &);
// FUNCTION: SONIC 0x420e90 UnloadCharTextures
void unload_character_textures(std::size_t character, TextureState &, const ReleaseTextureList &);
struct ClearServices {
    std::function<void()> release_regular_object_textures;
    std::function<void()> release_character_textures;
    std::function<void()> clear_texture_binding;
};
// NOTSA: logical reset results, not a layout overlay of the scattered source globals.
// Their wider use is unrecovered; the reset function establishes all nine values.
struct AuxiliaryWork {
    std::array<std::uint32_t, 9> values{};
    std::uint32_t limit = 50;
};
struct TrackState {
    std::int32_t current_song = -1, previous_song = -1, real_previous_song = -1;
    std::int32_t current_voice = -1;
    std::uint32_t song_queue_state = 0, voice_queue_state = 0;
};
struct StageState {
    std::int16_t &stage;
    std::int16_t &act;
    std::int16_t &previous_stage;
    std::uint32_t &previous_act;
};
// MSGC field consumed by queuing. This is not a guessed rendering payload/layout.
struct Message {
    std::uint8_t kind;
};
struct MessageState {
    std::uint32_t count = 0;
    std::array<const Message *, 4> entries{};
};
inline constexpr std::int32_t kInitialSequenceRunning = 1;
// FUNCTION: SONIC 0x457060 FUN_00457060
std::int32_t reset_auxiliary_work(AuxiliaryWork &);
// FUNCTION: SONIC 0x412ad0 FUN_00412ad0
void clear_sequence_running(std::int32_t &);
// FUNCTION: SONIC 0x4257c0 FUN_004257c0
void reset_song_state(TrackState &);
// FUNCTION: SONIC 0x4257e0 ResetCurrentVoice
void reset_current_voice(TrackState &);
// FUNCTION: SONIC 0x414570 SetStageNumber
void set_stage_number(std::uint8_t stage, std::uint8_t act, StageState &);
// FUNCTION: SONIC 0x40d490 MSG_Disp
void display_message(const Message *, MessageState &, const std::uint8_t &control_message_gate,
                      const std::int32_t &pending_logic_ticks);
struct TitleState {
    std::int32_t &demo_mode;
    std::int32_t &demo_event;
    std::int32_t &demo_reset;
    std::int32_t &demo_enabled;
    std::int32_t &demo_paused;
    AuxiliaryWork &auxiliary;
    std::int32_t &sequence_running;
    std::uint8_t &hide_hud;
    std::uint32_t &pause_state;
    std::int32_t &story_event_mode;
    save::Data &save;
    save::Data &backup;
};
struct TitleServices {
    std::function<void()> initialize_heap;
    std::function<void()> initialize_tasks;
    std::function<void()> load_save;
    std::function<void(std::uint32_t)> set_fade_color;
    std::function<void(std::uint8_t, std::uint8_t)> set_stage_number;
};
// FUNCTION: SONIC 0x40bf30 Clear
void clear(ClearServices &);
// FUNCTION: SONIC 0x42ca10 FUN_0042ca10
void prepare_title(TitleState &, TitleServices &);
// FUNCTION: SONIC 0x40bf40 Reset
void reset(State &, Services &);
} // namespace sadx::cleanup
