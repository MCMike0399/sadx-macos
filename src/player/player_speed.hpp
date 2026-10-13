#pragma once

#include "player/player_state.hpp"
#include "player/player_physics.hpp"

#include <functional>
#include <optional>

namespace sadx {

enum class PlayerSpeedFailure {
    none,
    missing_input_gate,
    input_gate_unavailable,
    missing_task_to_world_transform,
    task_to_world_transform_unavailable,
    missing_floor_to_world_transform,
    floor_to_world_transform_unavailable,
    unavailable_floor_normal,
};

struct PlayerInputGateResult {
    PlayerSpeedFailure failure;
    std::optional<bool> active;
};

struct PlayerSpeedResult {
    PlayerSpeedFailure failure = PlayerSpeedFailure::none;
    // Original call-site VA identifying an unresolved boundary. Earlier stores
    // remain in caller-owned state; this result does not claim transactional
    // execution or that re-running from partially updated state is safe.
    std::uint32_t source_boundary = 0;
    std::optional<NinjaVector> temporary_vector;
    explicit operator bool() const noexcept { return failure == PlayerSpeedFailure::none; }
};

struct PlayerSpeedCallees {
    // 0x43bdc0(task, nullptr, nullptr), including stage/controller/global
    // context. The owning closure must provide it; no false/zero fallback.
    std::function<PlayerInputGateResult(const PlayerOrientation&, X87Context&)> input_gate;
    // 0x43ec90. The callable owns any required native matrix/table state.
    std::function<PlayerSpeedFailure(const PlayerOrientation&, NinjaVector&, X87Context&)> task_to_world;
    // 0x43edd0(floor_normal, vector, task_yaw).
    std::function<PlayerSpeedFailure(const NinjaVector&, NinjaVector&, std::int32_t, X87Context&)> floor_to_world;
};

// Full recovered local PGetSpeed 0x443f50 control/math body, plus observed tail
// add/limit/small-speed cleanup semantics. Required input/rotation callees remain
// explicit integration boundaries. This is NOT complete character physics.
// Native ext80 retains source intermediate precision and source binary32 stores.
PlayerSpeedResult get_player_speed(const PlayerOrientation& orientation,
                                   MotionWork& world_motion, PlayerPhysicsState& player,
                                   const PlayerParameters& parameters,
                                   const PlayerSpeedCallees& callees, X87Context& context);

} // namespace sadx
