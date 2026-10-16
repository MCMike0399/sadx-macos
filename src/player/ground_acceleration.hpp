#pragma once

#include "player/player_state.hpp"
#include <functional>
#include <stdexcept>

namespace sadx {
class PlayerCoordinates;
namespace physics {

// NOTSA: borrowed plain values, shared with the session's task and motion owners.
// References keep mutations by rotation/input owners visible within the same step.
struct GroundMotion {
    PlayerOrientation &orientation;
    MotionWork &world;
    PlayerMotion &player;
    float &alignment;
    std::uint32_t &attributes;
    std::int32_t &target_yaw;
    float &force;             // motionwk2.force
    float &friction_overflow; // motionwk2.accel; observed friction excess
    float &friction_scale;
};
struct GroundInput {
    std::int32_t angle;
    float magnitude;
    bool active;
};
// NOTSA: native composition supplies real dependency owners and their live facts.
// The input owner performs its own arithmetic/effects when queried, before contact reads.
struct GroundEnvironment {
    NinjaVector gravity;
    NinjaVector contact_direction;
    PlayerCoordinates *coordinates;
    std::function<GroundInput(X87Context &)> query_input;
};
// NOTSA: an uncomposed native operation stops at its semantic name.
class UnavailableGroundOperation : public std::runtime_error {
  public:
    explicit UnavailableGroundOperation(const char *name) : std::runtime_error(name) {}
};

// FUNCTION: SONIC 0x44c270 PGetAcceleration
void calculate_ground_acceleration(GroundMotion &, const PlayerParameters &,
                                   const GroundEnvironment &, X87Context &);
} // namespace physics
} // namespace sadx
