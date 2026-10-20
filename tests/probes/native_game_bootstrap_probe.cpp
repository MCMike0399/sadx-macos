#include "game/game_bootstrap.hpp"
#include <algorithm>
#include <cstring>

namespace {
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
}
extern "C" void sadx_native_game_bootstrap(const std::uint32_t *in, std::uint32_t *out) {
    using namespace sadx;
    std::int32_t card_result = 0;
    std::uint32_t global_timer = 0, movie_number = 0;
    std::uint16_t game_mode = 0;
    std::int16_t rings = 0;
    bootstrap::State state({card_result, global_timer, rings, movie_number, game_mode});
    TaskScheduler tasks;
    state.cutscene_task = tasks.append_task(0, [](TaskScheduler &, const TaskHandle &) {}).task;
    std::memset(state.sequences.data(), 165, sizeof(state.sequences));
    state.sequence_variables.fill(165);
    state.event_work.fill(165);
    state.cutscene_flags.fill(165);
    state.completed_stages.fill(165);
    state.section_work.fill(165);
    for (auto &record : state.character_work.records)
        record.fill(165);
    save::Data save;
    save.options = static_cast<std::uint8_t>(in[1]);
    save.lives.fill(static_cast<std::uint8_t>(in[2]));
    bootstrap::Services services{[](std::uint16_t) {}, [](std::uint32_t) {}};
    bootstrap::start(state, save, {{in[3]}, {in[4]}}, static_cast<std::int16_t>(in[0]), services);
    std::fill_n(out, 8192, 0);
    const std::uint32_t scalar[]{
        state.card_state, state.card_operation, static_cast<std::uint32_t>(state.card_result),
        state.card_complete,
        state.save_name == nullptr, state.character, static_cast<std::uint8_t>(state.lives),
        static_cast<std::uint32_t>(state.actions.clear_pose.identity),
        static_cast<std::uint32_t>(state.actions.clear_pose_transition.identity),
        state.global_timer, state.game_timer, state.frames, state.seconds, state.minutes,
        static_cast<std::uint16_t>(state.rings), state.ring_adjustment, state.saved_option_gate,
        state.cutscene_work, !state.cutscene_task, state.cutscene_control,
        state.cutscene_active, static_cast<std::uint32_t>(state.current_event),
        static_cast<std::uint32_t>(state.story_event_mode),
        static_cast<std::uint16_t>(state.game_mode),
        static_cast<std::uint16_t>(state.game_mode_change),
        static_cast<std::uint32_t>(state.movie_number),
        static_cast<std::uint16_t>(state.pause_selection), state.field_start_position == nullptr,
        bits(state.gravity[0]), bits(state.gravity[1]), bits(state.gravity[2]),
        static_cast<std::uint32_t>(state.gravity_angle_x),
        static_cast<std::uint32_t>(state.gravity_angle_z), state.perfect_chaos_debug,
        state.material_callback == bootstrap::MaterialCallback::CorrectMaterial,
        state.random_seed, state.current_sequence == nullptr, state.current_section == nullptr,
        state.character_work.first_index, state.character_work.second_index,
        state.character_work.third_index, state.character_work.marker,
        state.character_work.control, state.character_work.entry_count,
        state.character_work.active, state.character_work.counter, state.character_work.work_value};
    auto cursor = std::copy(std::begin(scalar), std::end(scalar), out);
    for (const auto &entry : state.character_work.entries) {
        *cursor++ = entry.marker;
        *cursor++ = bits(entry.factor);
    }
    const auto append = [&](const auto &array) {
        static_assert(sizeof(array) % 4 == 0);
        std::memcpy(cursor, array.data(), sizeof(array));
        cursor += sizeof(array) / 4;
    };
    append(state.sequences);
    append(state.sequence_variables);
    append(state.event_work);
    append(state.cutscene_flags);
    append(state.completed_stages);
    append(state.section_work);
    append(state.character_work.records);
}
extern "C" std::uint32_t sadx_native_game_bootstrap_result(const std::uint32_t *in) {
    std::int32_t card_result = 0;
    std::uint32_t global_timer = 0, movie = 0;
    std::int16_t rings = 0;
    std::uint16_t mode = 0;
    sadx::bootstrap::State state({card_result, global_timer, rings, movie, mode});
    sadx::save::Data save;
    save.options = static_cast<std::uint8_t>(in[1]);
    state.lives = static_cast<std::int8_t>(in[2]);
    sadx::bootstrap::Services services{[](std::uint16_t) {}, [](std::uint32_t) {}};
    const auto stage = static_cast<std::int16_t>(in[0]);
    return sadx::bootstrap::initialize_game(state, save, stage, services);
}
extern "C" void sadx_native_game_bootstrap_menu(std::uint32_t *out) {
    using namespace sadx;
    bootstrap::MenuState state;
    state.flags.fill(165);
    state.selected_storage = state.selected_file = state.character_cursor = 165;
    state.title_count = state.player_character = state.stage = state.act = 165;
    state.player_number = 42405;
    TaskScheduler tasks;
    state.tails_ai = tasks.append_task(0, [](TaskScheduler &, const TaskHandle &) {}).task;
    std::uint8_t main_mode = 165;
    bootstrap::initialize_advertise(state, main_mode);
    std::copy(state.flags.begin(), state.flags.end(), out);
    const std::uint32_t fields[]{state.selected_storage, state.selected_file, main_mode,
                                 state.character_cursor, state.title_count, state.player_character,
                                 state.stage, state.act, state.player_number, !state.tails_ai,
                                 bootstrap::loading_status(), bootstrap::loading_ready()};
    std::copy(std::begin(fields), std::end(fields), out + 8);
}
