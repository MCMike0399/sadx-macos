#pragma once
#include "cpu/fidelity_float.hpp"
#include "player/ground_acceleration.hpp"

namespace sadx::physics {
// NOTSA: compiler-pool literals, named for their roles; reference builds may supply patched bits.
struct GroundConstants {
    float zero = 0.0f;
    float inactive_slide_multiplier = 10.0f;
    float half = 0.5f;
    float steep_slope_weight_multiplier = 5.0f;
    float turn_reduction = 0.8f;
    float high_speed_input_reduction = 0.3f;
    float low_input_reduction = 0.4f;
    float cross_slope_alignment = 0.1f;
    float medium_input_threshold = 0.7f;
    float cross_slope_threshold = 0.6f;
    float high_input_threshold = 0.9f;
    float shallow_negative_alignment = -0.1f;
    float moderate_slope_side_multiplier = 1.4f;
    float overspeed_alignment = 0.96f;
    float steep_negative_alignment = -0.4f;
    float shallow_slope_alignment = 0.87f;
    float slide_alignment = 0.71f;
    float stop_minimum = -0.051f;
    float stop_maximum = 0.051f;
    float slide_speed = 1.16f;
    float overspeed_drag_multiplier = 1.7f;
    float attribute_alignment = 0.94f;
    float side_slope_multiplier = 4.225f;
    float medium_negative_alignment = -0.3f;
};
// NOTSA: working acceleration is formed before transform/input/rotation and published on stores.
struct GroundContext {
    GroundMotion &motion;
    const PlayerParameters &parameters;
    const GroundEnvironment &environment;
    X87Context &fp;
    NinjaVector acceleration{};
    GroundInput input{};
};
enum class GroundYaw { Fast, BlendedSmallTurn, BlendedAttribute, Regular };
enum class InputGainBand { Jog, Run, Fast, AboveMaximumNegative, AboveMaximumOther };
enum class GroundFriction { Apply, SkipLongitudinal };
// NOTSA: friction borrows only fields used by the common ground/alternate tail.
struct GroundFrictionState {
    PlayerMotion &motion;
    std::uint32_t &attributes;
    float &overflow;
    float &scale;
};
// NOTSA: recovered common friction tail, shared by ordinary and alternate acceleration.
void finish_ground_friction(NinjaVector &, float drive, float extra_y, bool active,
                            GroundFrictionState, const PlayerParameters &, const GroundConstants &,
                            GroundFriction, X87Context &);
// NOTSA: semantic dependency/composition seams; source diagnostics live in the adapter.
const GroundConstants &ground_constants(GroundContext &);
const NinjaVector &read_world_gravity(GroundContext &);
void convert_gravity_to_player(GroundContext &);
bool query_ground_input(GroundContext &);
std::int32_t ground_input_angle(GroundContext &);
float ground_input_magnitude(GroundContext &, InputGainBand);
NinjaVector ground_contact_direction(GroundContext &);
// NOTSA: observation of the local acceleration, with no native gameplay effect.
void report_ground_acceleration(GroundContext &);
// NOTSA: composition seams to the existing recovered mutation owners; native bindings remain open.
void event_ground_acceleration(GroundContext &);
void attribute_ground_acceleration(GroundContext &);
void alternate_ground_acceleration(GroundContext &);
void passive_ground_rotation(GroundContext &);
void reset_ground_direction(GroundContext &);
void rotate_ground_attribute(GroundContext &);
void turn_ground_yaw(GroundContext &, std::int32_t, GroundYaw);
} // namespace sadx::physics
