#include "player/player_state.hpp"
#include <cstring>
#include <limits>

namespace sadx {
namespace {
std::uint32_t read_word(const std::uint8_t* bytes) {
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8) |
           (static_cast<std::uint32_t>(bytes[2]) << 16) |
           (static_cast<std::uint32_t>(bytes[3]) << 24);
}
float read_float(const std::uint8_t* bytes) {
    const auto word = read_word(bytes);
    float result;
    std::memcpy(&result, &word, 4);
    return result;
}
}

ParameterResult decode_player_parameters(const std::uint8_t* bytes, std::size_t size) {
    static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
    if (!bytes || size != 132) return {PlayerSeedFailure::invalid_parameter_bytes, {}};
    PlayerParameters parameters{};
    const auto timer = read_word(bytes);
    std::memcpy(&parameters.jump2_timer, &timer, 4);
    const std::array<float*, 32> fields{
        &parameters.pos_error, &parameters.lim_h_spd, &parameters.lim_v_spd,
        &parameters.max_x_spd, &parameters.max_psh_spd, &parameters.jmp_y_spd,
        &parameters.nocon_speed, &parameters.slide_speed, &parameters.jog_speed,
        &parameters.run_speed, &parameters.rush_speed, &parameters.crash_speed,
        &parameters.dash_speed, &parameters.jmp_addit, &parameters.run_accel,
        &parameters.air_accel, &parameters.slow_down, &parameters.run_break,
        &parameters.air_break, &parameters.air_resist_air, &parameters.air_resist,
        &parameters.air_resist_y, &parameters.air_resist_z, &parameters.grd_frict,
        &parameters.grd_frict_z, &parameters.lim_frict, &parameters.rat_bound,
        &parameters.rad, &parameters.height, &parameters.weight,
        &parameters.eyes_height, &parameters.center_height};
    for (std::size_t i=0; i<fields.size(); ++i) *fields[i] = read_float(bytes+4+i*4);
    return {PlayerSeedFailure::none, parameters};
}

PlayerSeedResult seed_player_fields(std::uint8_t character_row,
                                   std::uint8_t existing_task_id,
                                   const PlayerParameters& parameters,
                                   const PlayerEventFlags& events,
                                   X87Context& context) {
    if (character_row > 7) return {PlayerSeedFailure::unknown_character_row, {}};
    std::uint16_t equipment = 0;
    const auto apply = [&](std::size_t flag, std::uint16_t mask) {
        // GetEventFlag 0x412d20 uses nonzero byte, not a particular bit/value.
        if (events[flag] != 0) equipment |= mask;
    };
    switch (character_row) {
    case 0:
        apply(0x8d, 0x2); apply(0x8e, 0x1); apply(0x9f, 0x4);
        [[fallthrough]];
    case 2:
        apply(0xcd, 0x8); apply(0xdb, 0x10); break;
    case 3:
        apply(0x119, 0x40); apply(0x11a, 0x20); break;
    case 5:
        apply(0x152, 0x100); apply(0x15e, 0x80); break;
    case 6:
        apply(0x189, 0x400); apply(0x18a, 0x200);
        [[fallthrough]];
    case 7:
        apply(0x1d0, 0x1000); apply(0x1d1, 0x800); break;
    default: break;
    }
    // 0x4427cd: parameter rad * exact float at 0x7dd58c (0.2f),
    // followed by a binary32 store to playerwk.shadow.scl (+0x1fc).
    const float shadow_scale = player_initial_shadow_scale(parameters.rad,context);
    return {PlayerSeedFailure::none,
            PlayerInitialFields{parameters, {{0,0,0},{0,0,0}}, equipment, 0, 0, 0,
                                existing_task_id ? existing_task_id : std::uint8_t{1},
                                {1,1,1}, 1.0F, shadow_scale}};
}

float player_initial_shadow_scale(float radius,X87Context& context) {
    return context.to_float(context.multiply_float_memory(context.from_float(radius),0.2F));
}
} // namespace sadx
