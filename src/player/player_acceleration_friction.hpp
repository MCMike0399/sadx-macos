#pragma once

#include "player/player_acceleration_ground.hpp"

namespace sadx {
enum class LongitudinalFriction { apply, skip };

// Source-identical ground0x44c97c..0x44cde5 / attribute20000
// 0x44b549..0x44b9b2 arithmetic and stores. Ordinary ground's special
// amplification jump enters at its Z-friction section, hence explicit skip.
// Retains ext80 versus binary32 spills and commits X before final Y spill.
void finish_player_ground_friction(NinjaVector& acceleration,float drive,float extra_y,
    bool input_active,PlayerMotionState&,PlayerPhysicsState&,const PlayerParameters&,
    const GroundAccelerationConstants&,LongitudinalFriction,X87Context&);
} // namespace sadx
