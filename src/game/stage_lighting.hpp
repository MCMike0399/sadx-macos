#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <utility>

namespace sadx::lighting {
// LE_LIGHT_ENV, 48 bytes in the source. Imported float words retain their bits.
struct Environment {
    std::uint8_t stage, act, light_index, flags;
    std::array<float, 3> direction;
    float specular, diffuse;
    std::array<float, 3> color, ambient;
};
// LE_PALIGHT_ENV, 96 bytes in the source.
struct PaletteEnvironment {
    std::uint8_t stage, act, character, flags;
    std::array<float, 3> direction;
    float diffuse;
    std::array<float, 3> ambient;
    float upper_color_power;
    std::array<float, 3> upper_color;
    float upper_specular_power;
    std::array<float, 3> upper_specular;
    float lower_color_power;
    std::array<float, 3> lower_color;
    float lower_specular_power;
    std::array<float, 3> lower_specular;
};
// Mutable native copies of the imported file-backed owners, including entry255
// beyond the SDK's declared arrays. Preserve its imported bytes: stage init
// subsequently writes a sentinel there; the original table already ends earlier.
struct Tables {
    Environment default_stage, default_character;
    std::array<Environment, 256> stages;
    std::array<PaletteEnvironment, 256> palettes;
};
struct State {
    explicit State(Tables imported) : tables(std::move(imported)) {}
    Tables tables;
    // These live selections and flags begin in the source's zero-filled storage.
    std::array<Environment, 4> active_stage{};
    Environment active_character{};
    PaletteEnvironment active_palette{};
    std::uint8_t dirty = 0;
    std::int32_t sky_rotation = 0, rotation_y = 0, rotation_z = 0;
};
struct StageView {
    std::int16_t &stage;
    std::int16_t &act;
};
struct TimeServices {
    std::function<std::int32_t()> sequence_time;
    std::function<std::int32_t()> hotel_state;
};
// FUNCTION: SONIC 0x412c10 SeqGetTime
std::int32_t sequence_time(const std::int8_t *current_time, std::int32_t demo_event);
// FUNCTION: SONIC 0x40a420 FUN_0040a420
void adjust_time_of_day(std::int32_t &stage, std::uint32_t &act, TimeServices &);
// FUNCTION: SONIC 0x40a4a0 GetTimeOfDayLevelAndAct
std::pair<std::int32_t, std::uint32_t> time_of_day_stage(StageView, TimeServices &);
// FUNCTION: SONIC 0x40a8a0 InitDefaultStageLight
void initialize_default_stage(State &);
// FUNCTION: SONIC 0x40a950 SetCurrentStageLights
void select_stage(std::uint32_t stage, std::uint32_t act, State &);
// FUNCTION: SONIC 0x40aa30 FUN_0040aa30
const PaletteEnvironment *find_palette(std::uint32_t stage, std::uint32_t act,
                                        std::int32_t character, const Tables &);
// FUNCTION: SONIC 0x40aab0 SetCurrentLSPalette
void select_palette(std::uint32_t stage, std::uint32_t act, std::int32_t character, State &);
// FUNCTION: SONIC 0x40ab60 GetAndSetCurrentLSPalette
std::int32_t select_palettes(std::uint16_t stage, std::uint16_t act, State &);
// FUNCTION: SONIC 0x40ad60 DoCharacterLightStuff_B
void select_character(std::int32_t stage, std::uint32_t act, State &, TimeServices &);
// FUNCTION: SONIC 0x40aeb0 FUN_0040aeb0
void initialize_render_state(StageView, State &, TimeServices &);
// FUNCTION: SONIC 0x40aee0 FUN_0040aee0
void update_character_light(StageView, State &, TimeServices &);
} // namespace sadx::lighting
