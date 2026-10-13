#pragma once

#include "player/player_motion.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace sadx {

// Decoded values from the 132-byte parameter records at guest VA 0x9154e8.
// This contains no guest pointers and is not a cast of the PE's memory.
struct PlayerParameters {
    std::int32_t jump2_timer;
    float pos_error, lim_h_spd, lim_v_spd, max_x_spd, max_psh_spd;
    float jmp_y_spd, nocon_speed, slide_speed, jog_speed, run_speed;
    float rush_speed, crash_speed, dash_speed, jmp_addit, run_accel;
    float air_accel, slow_down, run_break, air_break, air_resist_air;
    float air_resist, air_resist_y, air_resist_z, grd_frict, grd_frict_z;
    float lim_frict, rat_bound, rad, height, weight, eyes_height, center_height;
};

enum class PlayerSeedFailure { none, invalid_parameter_bytes, unknown_character_row };
struct ParameterResult {
    PlayerSeedFailure failure;
    std::optional<PlayerParameters> parameters;
    explicit operator bool() const noexcept { return failure == PlayerSeedFailure::none; }
};

// Preserve source bit patterns, including signed zeros and nonfinite floats;
// numeric validation belongs to the operation that consumes a parameter.
ParameterResult decode_player_parameters(const std::uint8_t* bytes, std::size_t size);

// Exact 0x4427cd..0x4427e5 operation, shared by the startup owner and the
// isolated seed helper. Source multiplier0x7dd58c has binary32 encoding0.2f.
float player_initial_shadow_scale(float radius, X87Context& context);

using PlayerEventFlags = std::array<std::uint8_t, 512>;

struct PlayerInitialFields {
    PlayerParameters parameters;
    PlayerMotion motion;
    std::uint16_t equipment;
    std::uint16_t island_number;
    std::int16_t animation_mode;
    std::int16_t join_vertex_mode;
    std::uint8_t task_id;
    NinjaVector task_scale;
    float motion_friction;
    float shadow_scale;
};
struct PlayerSeedResult {
    PlayerSeedFailure failure;
    std::optional<PlayerInitialFields> fields;
    explicit operator bool() const noexcept { return failure == PlayerSeedFailure::none; }
};

// Pure recovered field/upgrade initialization within PInitialize 0x442750.
// character rows are Sonic=0, Eggman=1, Tails=2, Knuckles=3, Tikal=4,
// Amy=5, Gamma=6, Big=7. No default save/event state is invented.
// This does NOT implement original allocation, start-position lookup, global
// player registration, collision/motion pointer binding, or task creation.
PlayerSeedResult seed_player_fields(std::uint8_t character_row,
                                   std::uint8_t existing_task_id,
                                   const PlayerParameters& parameters,
                                   const PlayerEventFlags& events,
                                   X87Context& context);

} // namespace sadx
