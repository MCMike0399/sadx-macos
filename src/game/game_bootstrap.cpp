#include "game/game_bootstrap.hpp"
#include <stdexcept>

namespace sadx::bootstrap {
void load_sonic_actions(State &state, const SonicActions &actions) {
    state.actions.clear_pose = actions.clear_pose;
    state.actions.clear_pose_transition = actions.clear_pose_transition;
}
void load_player_motion_data(std::uint16_t character, State &state,
                             const SonicActions &actions) {
    // Source cases 0 and 2 share this action pair. Other characters need their real pack.
    if (character != 0 && character != 2)
        throw std::runtime_error("LoadPlayerMotionData requires unrecovered character resources");
    load_sonic_actions(state, actions);
}
std::int16_t player_lives(const State &state) { return state.lives; }
void reset_lives(State &state) { state.lives = 4; }
void initialize_character_work(CharacterWork &work) {
    for (auto &entry : work.entries) {
        entry.marker = 255;
        entry.factor = 1;
    }
    work.first_index = 4294967295U;
    work.second_index = 4294967295U;
    work.third_index = 65535;
    for (auto &record : work.records)
        record.fill(0);
    work.control = 0;
    work.entry_count = 0;
    work.counter = 0;
    work.active = 1;
    for (auto &record : work.records)
        record[0] = 8;
    work.marker = 255;
    work.work_value = 0;
}
void setup_character(std::uint16_t character, State &state, save::Data &save,
                     const SonicActions &actions, Services &services) {
    services.unload_character_textures(state.character);
    state.character = character;
    load_player_motion_data(character, state, actions);
    auto index = std::uint32_t(state.character);
    if (index > 4)
        --index;
    if (index > 1)
        --index;
    state.lives = static_cast<std::int8_t>(save.lives[index]);
    if (player_lives(state) < 4)
        reset_lives(state);
    initialize_character_work(state.character_work);
}
void reset_memory_card(State &state) {
    state.card_state = 1;
    state.card_operation = 0;
    state.card_result = 0;
    state.card_complete = 0;
    state.save_name = nullptr;
}
void reset_time(State &state) {
    state.global_timer = 0;
    state.frames = 0;
    state.seconds = 0;
    state.minutes = 0;
    state.game_timer = 0;
    if (state.character == 6) {
        state.minutes = 3;
        state.seconds = 0;
    }
}
void reset_rings(State &state) {
    state.rings = 0;
    state.ring_adjustment = 0;
}
void initialize_cutscene_flags(State &state) {
    state.cutscene_flags.fill(0);
    state.cutscene_work = 0;
    state.cutscene_task = {};
    state.cutscene_control = 0;
    state.saved_option_gate = 1;
    state.cutscene_active = 0;
    state.current_event = -1;
}
void initialize_story_flags(State &state) {
    state.sequences.fill({});
    state.sequence_variables.fill(0);
    state.completed_stages.fill(0);
    state.section_work.fill(0);
    state.event_work.fill(0);
    state.current_sequence = nullptr;
    state.current_section = nullptr;
    state.sequence_variables[65] = 1;
    initialize_cutscene_flags(state);
    state.story_event_mode = 0;
}
void reset_gravity(State &state) {
    state.gravity = {0, -1, 0};
    state.gravity_angle_x = 0;
    state.gravity_angle_z = 0;
}
std::uint8_t initialize_game(State &state, const save::Data &save, const std::int16_t &stage,
                            Services &services) {
    state.game_mode = 21;
    state.game_mode_change = 0;
    state.movie_number = 0;
    reset_gravity(state);
    state.saved_option_gate = ~(save.options >> 1) & 1;
    state.pause_selection = 0;
    state.field_start_position = nullptr;
    if (player_lives(state) < 4)
        reset_lives(state);
    services.stop_vibration(0);
    state.material_callback = MaterialCallback::CorrectMaterial;
    if (stage <= 25)
        return 2;
    if (stage <= 34)
        return 1;
    return stage <= 38 ? 2 : 1;
}
void start(State &state, save::Data &save, const SonicActions &actions,
           const std::int16_t &stage, Services &services) {
    reset_memory_card(state);
    setup_character(0, state, save, actions, services);
    reset_time(state);
    reset_rings(state);
    initialize_story_flags(state);
    state.saved_option_gate = 0;
    initialize_game(state, save, stage, services);
    loading_status();
    state.perfect_chaos_debug = 0;
    state.random_seed = 3735882923U;
}
void initialize_advertise(MenuState &state, std::uint8_t &main_menu_mode) {
    state.flags.fill(0);
    state.selected_storage = 0;
    state.selected_file = 0;
    main_menu_mode = 0;
    state.character_cursor = 0;
    state.title_count = 0;
    state.player_character = 0;
    state.stage = 0;
    state.act = 0;
    state.player_number = 0;
    state.tails_ai = {};
}
std::uint32_t loading_status() { return 15; }
std::uint32_t loading_ready() { return 1; }
} // namespace sadx::bootstrap
