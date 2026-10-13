#pragma once

#include "player/player_acceleration.hpp"
#include "player/player_heading.hpp"
#include "player/player_rotation_result.hpp"

namespace sadx {

struct PlayerRotationConstants {
    static constexpr std::array<std::uint32_t,16> float_addresses{{
        0x7dfff0,0x7dd568,0x7dd550,0x7e0070,0x7e0118,0x7dcc98,
        0x7dcb10,0x7e011c,0x7e0120,0x7dce68,0x7dcb2c,0x7e0164,
        0x7dccf4,0x7e0160,0x7e0170,0x7dcb60}};
    static constexpr std::array<std::uint32_t,3> double_addresses{{0x7e0168,0x7dccd0,0x7dcce8}};
    std::array<std::uint32_t,16> float_bits;
    std::array<std::uint64_t,3> double_bits;
    float single(std::uint32_t address) const;
    double wide(std::uint32_t address) const;
};

struct PlayerRotationRuntime {
    // Passed playerwk +0x0a, distinct from GetAnalog's registered-player lookup.
    std::optional<std::uint16_t> passed_player_no_control_timer;
    std::optional<NinjaVector> world_gravity; // globals 0x3b0f0f8..0x3b0f100
    PlayerInputRuntime input;
    ForcedInputAngleConversion forced_angle;
};
// Actual source 0x443c50 / 0x443e60. Existing shared task/player/motion owners
// supply every field; speed stores and earlier yaw changes survive failures.
PlayerRotationResult rotate_player_regular(PlayerTaskState&,PlayerMotionState&,
    PlayerPhysicsState&,std::int32_t target,const PlayerRotationConstants&,
    const PlayerRotationRuntime&,PlayerCoordinates&,X87Context&);
PlayerRotationResult rotate_player_blended(PlayerTaskState&,PlayerPhysicsState&,
    const PlayerParameters&,std::int32_t target,const PlayerRotationConstants&,
    PlayerCoordinates&,X87Context&);

// Actual source 0x4491e0 / 0x449380, including source magnitude/normalization,
// selected native FPATAN/CRT conversion and real regular/fast yaw owners.
PlayerRotationResult rotate_player_passive(PlayerTaskState&,PlayerMotionState&,
    PlayerPhysicsState&,const PlayerParameters&,const PlayerRotationConstants&,
    const PlayerRotationRuntime&,PlayerCoordinates&,X87Context&);
PlayerRotationResult rotate_player_attribute(PlayerTaskState&,PlayerMotionState&,
    PlayerPhysicsState&,const PlayerParameters&,const PlayerRotationConstants&,
    const PlayerRotationRuntime&,PlayerCoordinates&,X87Context&);

// Recovered primitives 0x787cc0 and 0x418ac0. Their different sum/spill/zero
// policies are deliberate; no generic host normalize replaces either body.
X87Value rotation_vector_length(const NinjaVector&,X87Context&);

} // namespace sadx
