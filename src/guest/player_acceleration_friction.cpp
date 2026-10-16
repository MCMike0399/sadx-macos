#include "player/player_acceleration_friction.hpp"
#include "guest/ground_acceleration_reference.hpp"

namespace sadx {
void finish_player_ground_friction(NinjaVector &gravity, float drive, float extra_y, bool active,
                                   PlayerMotionState &world, PlayerPhysicsState &player,
                                   const PlayerParameters &parameters,
                                   const GroundAccelerationConstants &constants,
                                   LongitudinalFriction mode, X87Context &fp) {
    physics::finish_ground_friction(
        gravity, drive, extra_y, active,
        {player.motion, player.attributes, world.source_field_38, world.friction_scale}, parameters,
        decode_ground_constants(constants),
        mode == LongitudinalFriction::apply ? physics::GroundFriction::Apply
                                            : physics::GroundFriction::SkipLongitudinal,
        fp);
}
} // namespace sadx
