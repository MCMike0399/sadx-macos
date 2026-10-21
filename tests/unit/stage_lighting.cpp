#include "game/stage_lighting.hpp"
#include <cassert>

int main() {
    using namespace sadx::lighting;
    Tables tables{};
    for (auto &entry : tables.stages) entry.stage = 255;
    for (auto &entry : tables.palettes) entry.stage = 255;
    tables.stages[255].stage = 0; // preserve the actual import; init writes the extra sentinel
    tables.stages[0] = {0, 7, 2, 9, {1, 2, 3}, 4, 5, {6, 7, 8}, {9, 10, 11}};
    State state(tables);
    initialize_default_stage(state);
    assert(state.dirty == 1 && state.tables.stages[255].stage == 255);
    assert(state.tables.default_stage.stage == 0 && state.tables.default_stage.act == 0);
    assert(state.tables.default_stage.light_index == 0 && state.tables.default_stage.flags == 1);
    assert(state.tables.default_stage.direction[1] == 2 && state.tables.default_stage.ambient[2] == 11);
    state.tables.stages[0] = {1, 0, 4, 2, {1, 2, 3}, 4, 5, {6, 7, 8}, {9, 10, 11}};
    state.tables.stages[1] = {1, 2, 5, 3, {4, 5, 6}, 4, 5, {12, 13, 14}, {9, 10, 11}};
    state.tables.stages[2] = {1, 0, 9, 4, {7, 8, 9}, 4, 5, {15, 16, 17}, {9, 10, 11}};
    select_stage(1, 2, state);
    assert(state.active_stage[0].light_index == 0 && state.active_stage[0].act == 2);
    assert(state.active_stage[1].light_index == 1 && state.active_stage[1].color[0] == 15);
    assert(state.active_stage[1].flags == 4 && state.active_stage[1].act == 2);
    auto &palettes = state.tables.palettes;
    palettes[0] = {};
    palettes[0].stage = 1;
    palettes[0].act = 2;
    palettes[1] = palettes[0];
    palettes[1].flags = 3;
    palettes[1].direction = {10, 11, 12};
    palettes[2] = palettes[0];
    palettes[2].character = 6;
    palettes[2].flags = 1;
    assert(find_palette(1, 2, -8, state.tables) == &palettes[1]);
    assert(find_palette(1, 2, -6, state.tables) == &palettes[2]);
    select_palette(1, 2, -8, state);
    assert(state.active_palette.character == 8 && state.active_palette.direction[0] == 10);
    select_palette(1, 2, -6, state);
    assert(state.active_palette.character == 6 && state.active_palette.direction == state.active_stage[0].direction);
    palettes[3] = {};
    palettes[3].flags = 1;
    assert(select_palettes(1, 2, state) == 1);
    assert(state.active_palette.stage == 0 && state.active_palette.act == 0);

    unsigned sequence_calls = 0, hotel_calls = 0;
    TimeServices services{[&] { ++sequence_calls; return 2; }, [&] { ++hotel_calls; return 2; }};
    std::int32_t level = 26;
    std::uint32_t act = 4;
    adjust_time_of_day(level, act, services);
    assert(level == 26 && act == 0 && sequence_calls == 0 && hotel_calls == 1);
    level = 29;
    act = 5;
    adjust_time_of_day(level, act, services);
    assert(level == 1 && act == 0 && sequence_calls == 0);
    std::int16_t shared_stage = 0, shared_act = -1;
    const auto packed = time_of_day_stage({shared_stage, shared_act}, services);
    assert(packed.first == -1 && packed.second == 255);
    assert(sequence_time(nullptr, 18) == 1 && sequence_time(nullptr, 19) == 0);
    const std::int8_t night = -128;
    assert(sequence_time(&night, 18) == -128);
    sequence_calls = 0;
    services.sequence_time = [&] { return static_cast<std::int32_t>(sequence_calls++); };
    shared_stage = 26;
    shared_act = 0;
    update_character_light({shared_stage, shared_act}, state, services);
    assert(sequence_calls == 2 && state.active_stage[0].act == 1);
    initialize_render_state({shared_stage, shared_act}, state, services);
    assert(shared_stage == 0 && shared_act == 1 && sequence_calls == 2);
}
