#pragma once
#include "player/ground_acceleration_internal.hpp"
#include "player/player_acceleration_ground.hpp"

namespace sadx {
physics::GroundConstants decode_ground_constants(const GroundAccelerationConstants &);
std::array<std::uint32_t, 24> native_ground_constant_words();
} // namespace sadx
