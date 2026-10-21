#include "game/stage_lighting.hpp"

namespace sadx::lighting {
namespace {
constexpr std::int32_t kEmeraldCoast = 1;
constexpr std::int32_t kStationSquare = 26;
constexpr std::int32_t kEggCarrierOutside = 29;
constexpr std::int32_t kMysticRuins = 33;
} // namespace
std::int32_t sequence_time(const std::int8_t *current_time, std::int32_t demo_event) {
    return current_time != nullptr ? *current_time : demo_event == 18;
}
void adjust_time_of_day(std::int32_t &stage, std::uint32_t &act, TimeServices &services) {
    auto selected_stage = stage;
    auto selected_act = act;
    if (selected_stage == kStationSquare) {
        if (selected_act == 5) {
            selected_act = 0;
        } else if (selected_act != 2) {
            if (selected_act == 4 && services.hotel_state() == 2) {
                selected_act = 0;
            } else {
                const auto time = services.sequence_time();
                selected_act = time == 1 ? 1 : (time == 2 ? 3 : 0);
            }
        }
    } else if (selected_stage == kMysticRuins) {
        if (static_cast<std::int32_t>(selected_act) < 3)
            selected_act = static_cast<std::uint32_t>(services.sequence_time());
    } else if (selected_stage == kEggCarrierOutside) {
        selected_stage = kEmeraldCoast;
        selected_act = 0;
    }
    act = selected_act;
    stage = selected_stage;
}
std::pair<std::int32_t, std::uint32_t> time_of_day_stage(StageView view, TimeServices &services) {
    // Both source shorts are sign-extended before OR. High act bits can affect
    // the stage selected by the arithmetic shift, not just its low act byte.
    const auto packed = (static_cast<std::uint32_t>(static_cast<std::int32_t>(view.stage)) << 8) |
                        static_cast<std::uint32_t>(static_cast<std::int32_t>(view.act));
    auto stage = static_cast<std::int32_t>(packed) >> 8;
    auto act = packed & 255U;
    if (stage == kStationSquare || stage == kMysticRuins || stage == kEggCarrierOutside)
        adjust_time_of_day(stage, act, services);
    return {stage, act};
}
void initialize_default_stage(State &state) {
    state.dirty = 1;
    if (state.tables.stages[0].stage == 0) {
        state.tables.default_stage = state.tables.stages[0];
        state.tables.default_stage.stage = 0;
        state.tables.default_stage.act = 0;
        state.tables.default_stage.light_index = 0;
        state.tables.default_stage.flags = 1;
    }
    state.tables.stages[0].stage = 0;
    state.tables.stages[255].stage = 255;
}
void select_stage(std::uint32_t stage, std::uint32_t act, State &state) {
    state.active_stage = {};
    state.active_stage[0] = state.tables.default_stage;
    auto &first = state.active_stage[0];
    first.stage = static_cast<std::uint8_t>(stage);
    first.act = static_cast<std::uint8_t>(act);
    first.light_index = 0;
    first.flags = 1;
    for (std::size_t index = 1; index < state.active_stage.size(); ++index)
        state.active_stage[index].light_index = static_cast<std::uint8_t>(index);
    for (std::size_t index = 0; index < 255; ++index) {
        const auto &entry = state.tables.stages[index];
        if (entry.stage == 255)
            break;
        if (entry.stage == stage && (entry.act == act || entry.act == 0)) {
            const auto light = entry.light_index & 3U;
            state.active_stage[light] = entry;
            auto &selected = state.active_stage[light];
            selected.stage = static_cast<std::uint8_t>(stage);
            selected.act = static_cast<std::uint8_t>(act);
            selected.light_index = static_cast<std::uint8_t>(light);
        }
    }
}
const PaletteEnvironment *find_palette(std::uint32_t stage, std::uint32_t act,
                                        std::int32_t character, const Tables &tables) {
    const auto negative = character < 0;
    const auto requested = negative ? 0U - static_cast<std::uint32_t>(character) :
                                     static_cast<std::uint32_t>(character);
    const PaletteEnvironment *fallback = nullptr;
    for (std::size_t index = 0; index < 255; ++index) {
        const auto &entry = tables.palettes[index];
        if (entry.stage == 255)
            break;
        if (entry.stage == stage && entry.act == act) {
            if (entry.character == requested)
                return &entry;
            if (negative && entry.character == 0)
                fallback = &entry;
        }
    }
    return fallback;
}
void select_palette(std::uint32_t stage, std::uint32_t act, std::int32_t character, State &state) {
    state.active_palette = {};
    auto *selected = find_palette(stage, act, character, state.tables);
    if (selected == nullptr)
        selected = find_palette(stage, 0, character, state.tables);
    if (selected == nullptr)
        selected = find_palette(0, 0, character, state.tables);
    if (selected != nullptr) {
        state.active_palette = *selected;
        const auto magnitude = character < 0 ? 0U - static_cast<std::uint32_t>(character) :
                                             static_cast<std::uint32_t>(character);
        if (magnitude == 6 || magnitude == 8)
            state.active_palette.character = static_cast<std::uint8_t>(magnitude);
    }
    if ((state.active_palette.flags & 2) == 0)
        state.active_palette.direction = state.active_stage[0].direction;
}
std::int32_t select_palettes(std::uint16_t stage, std::uint16_t act, State &state) {
    auto *entry = find_palette(stage, act, 0, state.tables);
    if (entry == nullptr || (entry->flags & 1) == 0) {
        act = 0;
        entry = find_palette(stage, act, 0, state.tables);
        if (entry == nullptr)
            stage = 0;
    }
    entry = find_palette(stage, act, -8, state.tables);
    select_palette(stage, act, entry != nullptr && (entry->flags & 1) != 0 ? -8 : 0, state);
    entry = find_palette(stage, act, -6, state.tables);
    select_palette(stage, act, entry != nullptr && (entry->flags & 1) != 0 ? -6 : 0, state);
    select_palette(stage, act, 0, state);
    return 1;
}
void select_character(std::int32_t stage, std::uint32_t act, State &state, TimeServices &services) {
    // GetPlayerNumber is a recovered pure load whose unused result is discarded.
    adjust_time_of_day(stage, act, services);
    state.sky_rotation = 0;
    state.rotation_y = 0;
    state.rotation_z = 0;
    select_stage(static_cast<std::uint32_t>(stage), act, state);
    state.active_character = state.tables.default_character;
    select_palettes(static_cast<std::uint16_t>(stage), static_cast<std::uint16_t>(act), state);
}
void initialize_render_state(StageView stage, State &state, TimeServices &services) {
    stage.stage = 0;
    stage.act = 1;
    initialize_default_stage(state);
    select_character(0, 0, state, services);
}
void update_character_light(StageView stage, State &state, TimeServices &services) {
    const auto selection = time_of_day_stage(stage, services);
    select_character(selection.first, selection.second, state, services);
}
} // namespace sadx::lighting
