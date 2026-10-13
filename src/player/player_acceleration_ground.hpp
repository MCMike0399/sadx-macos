#pragma once

#include "player/player_acceleration.hpp"
#include "player/player_heading.hpp"

namespace sadx {

// Source-bound binary32 constants, in the exact address sequence below.
// Bits are caller-supplied, preserving patched/source values without a default.
struct GroundAccelerationConstants {
    static constexpr std::array<std::uint32_t,24> addresses{{
        0x7dcb2c,0x7dcb34,0x7dcc98,0x7dccfc,0x7dce5c,0x7dd540,
        0x7dd550,0x7dd568,0x7dd578,0x7dd584,0x7dfff0,0x7e0064,
        0x7e0068,0x7e0088,0x7e00e4,0x7e0124,0x7e0138,0x7e0148,
        0x7e014c,0x7e0178,0x7e019c,0x7e01a0,0x7e01a4,0x7e01a8}};
    std::array<std::uint32_t,24> bits;
    float at(std::uint32_t source_address) const;
};
using GroundAccelerationMutation = std::function<PlayerAccelerationResult(
    PlayerTaskState&,PlayerMotionState&,PlayerPhysicsState&,
    const PlayerParameters&,X87Context&)>;
using GroundYawMutation = std::function<PlayerAccelerationResult(
    PlayerTaskState&,PlayerMotionState&,PlayerPhysicsState&,
    const PlayerParameters&,std::int32_t target_yaw,X87Context&)>;
struct GroundAccelerationCallees {
    // Source-local ownership adapters. These may mutate native game state;
    // missing/failed boundaries preserve earlier stores and never imply retry.
    GroundAccelerationMutation event_acceleration; // 0x447510
    GroundAccelerationMutation attribute_20000_acceleration; // 0x44b0e0
    GroundAccelerationMutation attribute_4000_acceleration; // 0x447010
    std::function<CoordinateResult(const PlayerOrientation&,NinjaVector&,X87Context&)> world_to_player;
    GroundAccelerationMutation passive_rotation; // 0x4491e0
    GroundAccelerationMutation direction_reset; // 0x43c130
    GroundAccelerationMutation attribute_rotation; // 0x449380
    GroundYawMutation yaw_fast; // 0x443df0
    GroundYawMutation yaw_blended; // 0x443e60
    GroundYawMutation yaw_regular; // 0x443c50
};

// Borrows immutable source constants and the caller's live runtime/coordinate
// owners. Their lifetime must cover this binding. Parameters and FP context
// come from each actual ground call, never from a captured physics snapshot.
GroundAccelerationMutation bind_ground_direction_reset(const PlayerDirectionResetConstants&,
    const PlayerDirectionResetRuntime&,PlayerCoordinates&);
GroundAccelerationMutation bind_ground_direction_reset(PlayerDirectionResetConstants&&,
    const PlayerDirectionResetRuntime&,PlayerCoordinates&)=delete;
GroundAccelerationMutation bind_ground_direction_reset(const PlayerDirectionResetConstants&,
    PlayerDirectionResetRuntime&&,PlayerCoordinates&)=delete;

// Complete local PGetAccelerationGround 0x44c270..0x44cde5, with explicit
// native callees for alternate acceleration and rotation owners. Preserves
// instruction-order binary32 stores, retained ext80, comparisons and partial
// mutation on an unresolved boundary. Does not normalize input/state values.
PlayerAccelerationResult get_player_acceleration_ground(
    PlayerTaskState&,PlayerMotionState&,PlayerPhysicsState&,
    const PlayerParameters&,const GroundAccelerationConstants&,
    const PlayerAccelerationRuntime&,const GroundAccelerationCallees&,X87Context&);

} // namespace sadx
