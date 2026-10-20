#include "game/game_mode_internal.hpp"

namespace sadx::flow {
void enter_phase(FlowContext &flow, FlowPhase phase) { flow.phase = phase; }
GameMode read_mode(FlowContext &flow) { return flow.state.mode; }
void write_mode(FlowContext &flow, GameMode value) { flow.state.mode = value; }
void write_pause_state(FlowContext &flow, std::uint32_t value) { flow.state.pause_state = value; }
std::int32_t read_save_phase(FlowContext &flow) { return flow.state.save_phase; }
void write_save_ready(FlowContext &flow, std::uint8_t value) { flow.state.save_ready = value; }
MenuResource read_message_resource(FlowContext &flow) { return flow.state.message_resource; }
void write_message_resource(FlowContext &flow, MenuResource value) {
    flow.state.message_resource = value;
}
std::uint8_t read_action_restart_requested(FlowContext &flow) {
    return flow.state.action_restart_requested;
}
void write_action_restart_requested(FlowContext &flow, std::uint8_t value) {
    flow.state.action_restart_requested = value;
}
void write_title_mode(FlowContext &flow, std::uint8_t value) { flow.state.title_mode = value; }
std::uint8_t read_title_load_phase(FlowContext &flow) { return flow.state.title_load_phase; }
void write_title_load_phase(FlowContext &flow, std::uint8_t value) {
    flow.state.title_load_phase = value;
}
void write_playing_super_sonic(FlowContext &flow, std::uint8_t value) {
    flow.state.playing_super_sonic = value;
}
std::uint8_t read_tutorial_variant(FlowContext &flow) { return flow.state.tutorial_variant; }
std::uint8_t read_alert_flag(FlowContext &flow) { return flow.state.alert_flag; }
std::uint8_t read_alert_blocked(FlowContext &flow) { return flow.state.alert_blocked; }
std::uint32_t read_movie_number(FlowContext &flow) { return flow.state.movie_number; }
void write_movie_number(FlowContext &flow, std::uint32_t value) { flow.state.movie_number = value; }
std::uint32_t read_menu_message_enabled(FlowContext &flow) {
    return flow.state.menu_message_enabled;
}
std::int16_t current_sequence_level(FlowContext &flow) {
    if (!flow.state.sequence)
        throw UnrecoveredGameFlowCall("current_sequence_level");
    return flow.state.sequence->level;
}
void initialize_game(FlowContext &flow) {
    if (!flow.native)
        throw UnrecoveredGameFlowCall("initialize_game");
    flow.native->initialize_game();
}
void initialize_demo_state(FlowContext &flow) {
    if (!flow.native)
        throw UnrecoveredGameFlowCall("initialize_demo_state");
    flow.native->initialize_demo_state();
}
std::uint32_t startup_status(FlowContext &flow) {
    if (!flow.native)
        throw UnrecoveredGameFlowCall("startup_status");
    return flow.native->startup_status();
}
std::int32_t startup_readiness(FlowContext &flow) {
    if (!flow.native)
        throw UnrecoveredGameFlowCall("startup_readiness");
    return flow.native->startup_readiness();
}
void request_file_module(FlowContext &, FlowModule) {
    throw UnrecoveredGameFlowCall("request_file_module");
}
void on_edit(FlowContext &) { throw UnrecoveredGameFlowCall("on_edit"); }
void initialize_sega_logo(FlowContext &flow) {
    if (!flow.native)
        throw UnrecoveredGameFlowCall("initialize_sega_logo");
    flow.native->initialize_sega_logo();
}
std::uint32_t run_logos(FlowContext &flow) {
    if (!flow.native)
        throw UnrecoveredGameFlowCall("run_logos");
    return flow.native->run_logos();
}
void set_frame_rate_mode(FlowContext &flow, std::int32_t mode, std::int32_t multiplier) {
    if (!flow.native)
        throw UnrecoveredGameFlowCall("set_frame_rate_mode");
    flow.native->set_frame_rate_mode(mode, multiplier);
}
std::uint32_t run_title(FlowContext &) { throw UnrecoveredGameFlowCall("run_title"); }
void load_regular_object_textures(FlowContext &, std::int32_t) {
    throw UnrecoveredGameFlowCall("load_regular_object_textures");
}
std::uint32_t initialize_level_game(FlowContext &flow) {
    if (!flow.native)
        throw UnrecoveredGameFlowCall("initialize_level_game");
    return flow.native->initialize_level_game();
}
void initialize_player_selection(FlowContext &) {
    throw UnrecoveredGameFlowCall("initialize_player_selection");
}
void get_switch_data(FlowContext &) { throw UnrecoveredGameFlowCall("get_switch_data"); }
void enable_pad_reading(FlowContext &) { throw UnrecoveredGameFlowCall("enable_pad_reading"); }
std::int16_t run_player_selection(FlowContext &) {
    throw UnrecoveredGameFlowCall("run_player_selection");
}
void clear_game(FlowContext &) { throw UnrecoveredGameFlowCall("clear_game"); }
void reset_game(FlowContext &) { throw UnrecoveredGameFlowCall("reset_game"); }
std::int16_t run_action(FlowContext &) { throw UnrecoveredGameFlowCall("run_action"); }
void prepare_action_return(FlowContext &) {
    throw UnrecoveredGameFlowCall("prepare_action_return");
}
std::uint32_t check_return_mode(FlowContext &) {
    throw UnrecoveredGameFlowCall("check_return_mode");
}
std::int16_t run_adventure(FlowContext &) { throw UnrecoveredGameFlowCall("run_adventure"); }
std::int32_t run_movie(FlowContext &flow, std::uint32_t movie) {
    if (!flow.native)
        throw UnrecoveredGameFlowCall("run_movie");
    return flow.native->run_movie(movie);
}
void enable_player_pad_reading(FlowContext &, std::uint32_t) {
    throw UnrecoveredGameFlowCall("enable_player_pad_reading");
}
std::int16_t run_mission(FlowContext &) { throw UnrecoveredGameFlowCall("run_mission"); }
void run_level_destructor(FlowContext &, std::uint32_t) {
    throw UnrecoveredGameFlowCall("run_level_destructor");
}
void free_message_resource(FlowContext &, MenuResource) {
    throw UnrecoveredGameFlowCall("free_message_resource");
}
void free_regular_object_texlists(FlowContext &) {
    throw UnrecoveredGameFlowCall("free_regular_object_texlists");
}
void free_character_texlists(FlowContext &) {
    throw UnrecoveredGameFlowCall("free_character_texlists");
}
void release_all_textures(FlowContext &) { throw UnrecoveredGameFlowCall("release_all_textures"); }
void prepare_title_assets(FlowContext &) { throw UnrecoveredGameFlowCall("prepare_title_assets"); }
void prepare_title_dialogs(FlowContext &) {
    throw UnrecoveredGameFlowCall("prepare_title_dialogs");
}
void initialize_title_menu(FlowContext &) {
    throw UnrecoveredGameFlowCall("initialize_title_menu");
}
void set_wait_vsync_count(FlowContext &flow, std::int32_t count) {
    if (!flow.native)
        throw UnrecoveredGameFlowCall("set_wait_vsync_count");
    flow.native->set_wait_vsync_count(count);
}
void load_level_sounds(FlowContext &) { throw UnrecoveredGameFlowCall("load_level_sounds"); }
std::int32_t run_title_menu(FlowContext &) { throw UnrecoveredGameFlowCall("run_title_menu"); }
void close_title_menu(FlowContext &, std::int32_t) {
    throw UnrecoveredGameFlowCall("close_title_menu");
}
void prepare_title(FlowContext &) { throw UnrecoveredGameFlowCall("prepare_title"); }
void start_screen_fade(FlowContext &) { throw UnrecoveredGameFlowCall("start_screen_fade"); }
std::uint32_t current_player_number(FlowContext &) {
    throw UnrecoveredGameFlowCall("current_player_number");
}
void setup_character(FlowContext &flow, std::uint32_t character) {
    if (!flow.native)
        throw UnrecoveredGameFlowCall("setup_character");
    flow.native->setup_character(character);
}
void reset_player_work(FlowContext &flow) {
    if (!flow.native)
        throw UnrecoveredGameFlowCall("reset_player_work");
    flow.native->reset_player_work();
}
void reset_player_parameters(FlowContext &) {
    throw UnrecoveredGameFlowCall("reset_player_parameters");
}
void load_save(FlowContext &) { throw UnrecoveredGameFlowCall("load_save"); }
void prepare_sequence(FlowContext &) { throw UnrecoveredGameFlowCall("prepare_sequence"); }
void set_sequence_player(FlowContext &, std::uint32_t) {
    throw UnrecoveredGameFlowCall("set_sequence_player");
}
void apply_sequence_level(FlowContext &, std::int16_t) {
    throw UnrecoveredGameFlowCall("apply_sequence_level");
}
void set_stage_number(FlowContext &, std::int32_t, std::int32_t) {
    throw UnrecoveredGameFlowCall("set_stage_number");
}
void set_entrance_number(FlowContext &, std::int32_t) {
    throw UnrecoveredGameFlowCall("set_entrance_number");
}
void set_sequence_time(FlowContext &, std::int32_t) {
    throw UnrecoveredGameFlowCall("set_sequence_time");
}
void go_demo_loop(FlowContext &) { throw UnrecoveredGameFlowCall("go_demo_loop"); }
std::uint32_t initialize_summary(FlowContext &, std::int32_t) {
    throw UnrecoveredGameFlowCall("initialize_summary");
}
std::uint32_t check_sequence_flag(FlowContext &, std::int32_t) {
    throw UnrecoveredGameFlowCall("check_sequence_flag");
}
std::uint32_t run_summary(FlowContext &, std::int32_t) {
    throw UnrecoveredGameFlowCall("run_summary");
}
std::uint32_t current_level_type(FlowContext &) {
    throw UnrecoveredGameFlowCall("current_level_type");
}
void initialize_tutorial(FlowContext &, std::int32_t) {
    throw UnrecoveredGameFlowCall("initialize_tutorial");
}
std::uint32_t run_tutorial(FlowContext &, std::int32_t) {
    throw UnrecoveredGameFlowCall("run_tutorial");
}
void initialize_staff_roll(FlowContext &, std::int32_t) {
    throw UnrecoveredGameFlowCall("initialize_staff_roll");
}
std::int32_t run_staff_roll(FlowContext &, std::int32_t) {
    throw UnrecoveredGameFlowCall("run_staff_roll");
}
void load_sega_logo(FlowContext &flow) {
    if (!flow.native)
        throw UnrecoveredGameFlowCall("load_sega_logo");
    flow.native->load_sega_logo();
}
void set_background_color(FlowContext &, Rgba, Rgba, Rgba) {
    throw UnrecoveredGameFlowCall("set_background_color");
}
void prepare_title_initialization(FlowContext &) {
    throw UnrecoveredGameFlowCall("prepare_title_initialization");
}
void stop_vibration(FlowContext &, std::uint32_t) {
    throw UnrecoveredGameFlowCall("stop_vibration");
}
void pause_all_sound(FlowContext &) { throw UnrecoveredGameFlowCall("pause_all_sound"); }
std::uint32_t mission_clear_status(FlowContext &) {
    throw UnrecoveredGameFlowCall("mission_clear_status");
}
void release_mission_clear_texlist(FlowContext &) {
    throw UnrecoveredGameFlowCall("release_mission_clear_texlist");
}
void display_message(FlowContext &) { throw UnrecoveredGameFlowCall("display_message"); }
void draw_screen_fade(FlowContext &flow) {
    if (!flow.native)
        throw UnrecoveredGameFlowCall("draw_screen_fade");
    flow.native->draw_screen_fade();
}
void increment_frame_counter(FlowContext &flow) {
    if (!flow.native)
        throw UnrecoveredGameFlowCall("increment_frame_counter");
    flow.native->increment_frame_counter();
}
} // namespace sadx::flow
