#include "player/ground_acceleration_internal.hpp"
#include "player/player_coordinates.hpp"

namespace sadx::physics {
const GroundConstants &ground_constants(GroundContext &) {
    static constexpr GroundConstants values{};
    return values;
}
const NinjaVector &read_world_gravity(GroundContext &context) {
    return context.environment.gravity;
}
void report_ground_acceleration(GroundContext &) {}
void convert_gravity_to_player(GroundContext &context) {
    if (!context.environment.coordinates)
        throw UnavailableGroundOperation("world_to_player");
    const auto result = context.environment.coordinates->world_to_player(
        context.motion.orientation, context.acceleration, context.fp);
    if (!result)
        throw UnavailableGroundOperation("world_to_player");
}
bool query_ground_input(GroundContext &context) {
    if (!context.environment.query_input)
        throw UnavailableGroundOperation("get_player_input");
    context.input = context.environment.query_input(context.fp);
    return context.input.active;
}
std::int32_t ground_input_angle(GroundContext &context) { return context.input.angle; }
float ground_input_magnitude(GroundContext &context, InputGainBand) {
    return context.input.magnitude;
}
NinjaVector ground_contact_direction(GroundContext &context) {
    return context.environment.contact_direction;
}
void event_ground_acceleration(GroundContext &) {
    throw UnavailableGroundOperation("event_acceleration");
}
void attribute_ground_acceleration(GroundContext &) {
    throw UnavailableGroundOperation("attribute_acceleration");
}
void alternate_ground_acceleration(GroundContext &) {
    throw UnavailableGroundOperation("alternate_acceleration");
}
void passive_ground_rotation(GroundContext &) {
    throw UnavailableGroundOperation("passive_rotation");
}
void reset_ground_direction(GroundContext &) {
    throw UnavailableGroundOperation("direction_reset");
}
void rotate_ground_attribute(GroundContext &) {
    throw UnavailableGroundOperation("attribute_rotation");
}
void turn_ground_yaw(GroundContext &, std::int32_t, GroundYaw) {
    throw UnavailableGroundOperation("turn_ground_yaw");
}
} // namespace sadx::physics
