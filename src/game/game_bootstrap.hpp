#pragma once
#include "engine/character_resources.hpp"
#include "engine/task.hpp"
#include "game/save_data.hpp"
#include <array>
#include <functional>

namespace sadx::bootstrap {
struct SonicActions {
    ActionHandle clear_pose, clear_pose_transition;
};
struct WorkEntry {
    std::uint8_t marker = 0;
    float factor = 0;
};
// NOTSA: fields written by SetupCharacter's work initializer; other roles are unrecovered.
struct CharacterWork {
    std::array<WorkEntry, 10> entries{};
    std::array<std::array<std::uint8_t, 512>, 26> records{};
    std::uint32_t first_index = 0, second_index = 0;
    std::uint16_t third_index = 0;
    std::uint8_t marker = 0, control = 0, entry_count = 0, active = 0;
    std::uint32_t counter = 0, work_value = 0;
};
struct SequenceSection;
struct FieldStartPosition;
// SEQUENCE, distinct from the persisted SAVE_DATA.SEQ_DATA record.
struct StorySequence {
    std::int8_t time = 0, emerald = 0;
    std::int16_t number = 0, section = 0, next_section = 0;
    std::uint16_t stage = 0, destination = 0;
};
enum class MaterialCallback { None, CorrectMaterial };
// ADVERTISE_WORK, with MainMenuSelectedMode borrowed from the flow owner.
struct MenuState {
    std::array<std::uint8_t, 8> flags{};
    std::uint8_t selected_storage = 0, selected_file = 0, character_cursor = 0;
    std::uint8_t title_count = 0, player_character = 0, stage = 0, act = 0;
    std::uint16_t player_number = 0;
    TaskHandle tails_ai;
};
// NOTSA: shared flow, frame, save and loop owners, borrowed without a second stored copy.
struct SharedState {
    std::int32_t &save_phase;
    std::uint32_t &global_timer;
    std::int16_t &rings;
    std::uint32_t &movie_number;
    std::uint16_t &game_mode;
};
struct State {
    explicit State(SharedState shared)
        : card_result(shared.save_phase), global_timer(shared.global_timer), rings(shared.rings),
          game_mode(shared.game_mode), movie_number(shared.movie_number) {}
    std::uint8_t card_state = 0, card_operation = 0, card_complete = 0;
    std::int32_t &card_result;
    const char *save_name = nullptr;
    std::uint16_t character = 0;
    std::int8_t lives = 0;
    SonicActions actions{};
    CharacterWork character_work;
    MenuState menu;
    std::uint32_t &global_timer;
    std::uint32_t game_timer = 0;
    std::uint8_t frames = 0, seconds = 0, minutes = 0;
    std::int16_t &rings;
    std::uint32_t ring_adjustment = 0;
    std::array<StorySequence, 8> sequences{};
    std::array<std::uint8_t, 512> sequence_variables{}, event_work{}, cutscene_flags{};
    std::array<std::uint8_t, 344> completed_stages{};
    // The three cleared section-work ranges overlap by20 bytes in the original.
    std::array<std::uint8_t, 932> section_work{};
    StorySequence *current_sequence = nullptr;
    SequenceSection *current_section = nullptr;
    std::uint32_t cutscene_work = 0;
    TaskHandle cutscene_task;
    std::uint8_t cutscene_control = 0, cutscene_active = 0;
    // Reset by cutscene initialization, then derived from saved option bit1; wider role unknown.
    std::uint8_t saved_option_gate = 0;
    std::int32_t current_event = 0, story_event_mode = 0;
    std::uint16_t &game_mode;
    std::int16_t game_mode_change = 0, pause_selection = 0;
    std::uint32_t &movie_number;
    FieldStartPosition *field_start_position = nullptr;
    std::array<float, 3> gravity{};
    std::int32_t gravity_angle_x = 0, gravity_angle_z = 0;
    std::uint8_t perfect_chaos_debug = 0;
    MaterialCallback material_callback = MaterialCallback::None;
    std::uint32_t random_seed = 0;
};
// NOTSA: resource and platform services; unavailable implementations must fail at the call.
struct Services {
    std::function<void(std::uint16_t)> unload_character_textures;
    std::function<void(std::uint32_t)> stop_vibration;
};
// FUNCTION: SONIC 0x494820 FUN_00494820
void load_sonic_actions(State &, const SonicActions &);
// FUNCTION: SONIC 0x422680 LoadPlayerMotionData
void load_player_motion_data(std::uint16_t character, State &, const SonicActions &);
// FUNCTION: SONIC 0x425fe0 GetNumPlayer
std::int16_t player_lives(const State &);
// FUNCTION: SONIC 0x425af0 ResetNumPlayer
void reset_lives(State &);
// FUNCTION: SONIC 0x46b230 FUN_0046b230
void initialize_character_work(CharacterWork &);
// FUNCTION: SONIC 0x414470 SetupCharacter
void setup_character(std::uint16_t character, State &, save::Data &, const SonicActions &,
                     Services &);
// FUNCTION: SONIC 0x421f90 FUN_00421f90
void reset_memory_card(State &);
// FUNCTION: SONIC 0x427f10 ResetTime2
void reset_time(State &);
// FUNCTION: SONIC 0x427ee0 FUN_00427ee0
void reset_rings(State &);
// FUNCTION: SONIC 0x42f5d0 FUN_0042f5d0
void initialize_cutscene_flags(State &);
// FUNCTION: SONIC 0x413300 InitFlagsAndThings
void initialize_story_flags(State &);
// FUNCTION: SONIC 0x43b490 ResetGravity
void reset_gravity(State &);
// FUNCTION: SONIC 0x413c00 InitGame
std::uint8_t initialize_game(State &, const save::Data &, const std::int16_t &stage, Services &);
// FUNCTION: SONIC 0x415000 FUN_00415000
void start(State &, save::Data &, const SonicActions &, const std::int16_t &stage, Services &);
// FUNCTION: SONIC 0x42cea0 FUN_0042cea0
void initialize_advertise(MenuState &, std::uint8_t &main_menu_mode);
// FUNCTION: SONIC 0x421f70 FUN_00421f70
std::uint32_t loading_status();
// FUNCTION: SONIC 0x421f80 FUN_00421f80
std::uint32_t loading_ready();
} // namespace sadx::bootstrap
