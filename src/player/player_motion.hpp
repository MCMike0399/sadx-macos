#pragma once

#include "math/ninja_math.hpp"

#include <array>
#include <cstdint>
#include <functional>

namespace sadx {

// Minimal native state. These objects are NOT packed guest-memory views.
// Every resource and value is supplied by its native owner; no guest pointer,
// default physics parameter, default gravity direction or inferred rotation.
struct MotionWork {
    NinjaVector speed;        // motionwk2 +0x04
    NinjaVector acceleration; // motionwk2 +0x10
};

struct PlayerMotion {
    NinjaVector acceleration; // playerwk +0x2c, in player-relative coordinates
    NinjaVector speed;        // playerwk +0x38
};

struct PlayerOrientation {
    std::uint16_t flags;                // taskwk +0x04
    std::array<std::int32_t, 3> angles; // taskwk +0x14, +0x18, +0x1c
};

struct MotionLimits {
    float horizontal; // playerwk +0xa4 = player_parameter.lim_h_spd
    float vertical;   // playerwk +0xa8 = player_parameter.lim_v_spd
};

enum class PlayerMotionFailure {
    none,
    missing_world_to_player_transform,
    world_to_player_transform_unavailable,
};

struct PlayerMotionResult {
    PlayerMotionFailure failure = PlayerMotionFailure::none;
    explicit operator bool() const noexcept { return failure == PlayerMotionFailure::none; }
};

// Required native boundary corresponding to 0x43ec00. It transforms the
// supplied temporary vector in place and returns explicit availability.
// std::function owns its callable/captured native state. No identity fallback.
using WorldToPlayerTransform = std::function<PlayerMotionResult(
    const PlayerOrientation&, NinjaVector&)>;

// PClearSpeed 0x43c550: independently optional owners, positive zero stores.
// Acceleration vectors are deliberately untouched.
void clear_speed(MotionWork* motion, PlayerMotion* player) noexcept;

// PGetGravity 0x443800: copy motion acceleration, add each world-gravity
// component multiplied by parameter weight (+0x114), transform temporary, and
// store player acceleration. Flag 0x4000 leaves state untouched, even if the
// transform is unavailable. Missing/failed transforms also leave state intact.
// Uses native software ext80 under the caller's explicit control word, then
// the observed binary32 stores. Actual game control state remains unverified.
PlayerMotionResult get_gravity(const PlayerOrientation& orientation,
                              const MotionWork& motion, PlayerMotion& player,
                              const NinjaVector& world_gravity, float weight,
                              const WorldToPlayerTransform& transform,
                              X87Context& context);

// Complete local semantics of 0x43c380, a PGetSpeed callee. X and Z clamp
// against horizontal, then Y against vertical. Comparisons are sequential,
// ordered and strict; source values including negative limits/NaN/signed zero
// are preserved rather than normalized. Both original flag-0x800 branches
// contain the same arithmetic, so no speculative flag-dependent mode exists.
void limit_motion_speed(MotionWork& motion, const MotionLimits& limits, X87Context& context);

// PGetSpeed 0x443f50 is owned by player_speed; these primitives alone do not
// provide its input/rotation/tail effects, collision or character transitions.

} // namespace sadx
