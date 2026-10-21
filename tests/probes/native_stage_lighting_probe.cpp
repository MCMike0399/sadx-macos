#include "game/stage_lighting.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace {
struct Failed { unsigned kind; };
}
extern "C" int sadx_native_stage_lighting(const std::uint32_t *in, const std::uint8_t *data,
                                          std::uint32_t *out) {
    using namespace sadx::lighting;
    static_assert(sizeof(Environment) == 48 && sizeof(PaletteEnvironment) == 96);
    static_assert(sizeof(Tables) == 36960 && std::is_trivially_copyable_v<Tables>);
    Tables tables;
    std::memcpy(&tables, data, sizeof(tables));
    State state(std::move(tables));
    std::memset(state.active_stage.data(), 165, sizeof(state.active_stage));
    std::memset(&state.active_character, 165, sizeof(state.active_character));
    std::memset(&state.active_palette, 165, sizeof(state.active_palette));
    state.dirty = 165;
    state.sky_rotation = state.rotation_y = state.rotation_z = static_cast<std::int32_t>(2779096485U);
    auto stage = static_cast<std::int16_t>(in[5]);
    auto act = static_cast<std::int16_t>(in[6]);
    unsigned seq_calls = 0, hotel_calls = 0;
    TimeServices services{[&] {
        ++seq_calls;
        if (in[10] && seq_calls + hotel_calls == in[10]) throw Failed{1};
        return static_cast<std::int32_t>(in[7] + seq_calls - 1);
    }, [&] {
        ++hotel_calls;
        if (in[10] && seq_calls + hotel_calls == in[10]) throw Failed{2};
        return static_cast<std::int32_t>(in[8]);
    }};
    std::fill_n(out, 9400, 0);
    auto selected_stage = static_cast<std::int32_t>(in[2]);
    auto selected_act = in[3];
    try {
        switch (in[0]) {
        case 0: initialize_default_stage(state); break;
        case 1: select_stage(in[2], in[3], state); break;
        case 2: {
            const auto *found = find_palette(in[2], in[3], static_cast<std::int32_t>(in[4]), state.tables);
            out[8] = found ? static_cast<std::uint32_t>(found - state.tables.palettes.data()) + 1 : 0;
            break;
        }
        case 3: select_palette(in[2], in[3], static_cast<std::int32_t>(in[4]), state); break;
        case 4: out[0] = select_palettes(static_cast<std::uint16_t>(in[2]), static_cast<std::uint16_t>(in[3]), state); break;
        case 5: select_character(static_cast<std::int32_t>(in[2]), in[3], state, services); break;
        case 6: initialize_render_state({stage, act}, state, services); break;
        case 7: update_character_light({stage, act}, state, services); break;
        case 8: adjust_time_of_day(selected_stage, selected_act, services); break;
        case 9: {
            const auto selected = time_of_day_stage({stage, act}, services);
            selected_stage = selected.first;
            selected_act = selected.second;
            break;
        }
        case 10: {
            const auto time = static_cast<std::int8_t>(in[7]);
            out[0] = static_cast<std::uint32_t>(sequence_time(in[11] ? &time : nullptr,
                                                            static_cast<std::int32_t>(in[12])));
            break;
        }
        }
    } catch (const Failed &failed) {
        out[7] = failed.kind;
    }
    out[1] = static_cast<std::uint32_t>(selected_stage);
    out[2] = selected_act;
    out[3] = static_cast<std::uint16_t>(stage);
    out[4] = static_cast<std::uint16_t>(act);
    out[5] = seq_calls;
    out[6] = hotel_calls;
    out[16] = state.dirty;
    out[17] = static_cast<std::uint32_t>(state.sky_rotation);
    out[18] = static_cast<std::uint32_t>(state.rotation_y);
    out[19] = static_cast<std::uint32_t>(state.rotation_z);
    auto *cursor = out + 32;
    const auto append = [&](const auto &value) {
        std::memcpy(cursor, &value, sizeof(value));
        cursor += sizeof(value) / 4;
    };
    append(state.tables);
    append(state.active_stage);
    append(state.active_character);
    append(state.active_palette);
    return 1;
}
