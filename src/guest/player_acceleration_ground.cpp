#include "guest/ground_acceleration_reference.hpp"
#include <cstring>
#include <stdexcept>

namespace sadx {
namespace {
struct ConstantField {
    std::uint32_t address;
    float physics::GroundConstants::*member;
};
constexpr ConstantField constant_fields[] = {
    {0x7dcb2c, &physics::GroundConstants::zero},
    {0x7dcb34, &physics::GroundConstants::inactive_slide_multiplier},
    {0x7dcc98, &physics::GroundConstants::half},
    {0x7dccfc, &physics::GroundConstants::steep_slope_weight_multiplier},
    {0x7dce5c, &physics::GroundConstants::turn_reduction},
    {0x7dd540, &physics::GroundConstants::high_speed_input_reduction},
    {0x7dd550, &physics::GroundConstants::low_input_reduction},
    {0x7dd568, &physics::GroundConstants::cross_slope_alignment},
    {0x7dd578, &physics::GroundConstants::medium_input_threshold},
    {0x7dd584, &physics::GroundConstants::cross_slope_threshold},
    {0x7dfff0, &physics::GroundConstants::high_input_threshold},
    {0x7e0064, &physics::GroundConstants::shallow_negative_alignment},
    {0x7e0068, &physics::GroundConstants::moderate_slope_side_multiplier},
    {0x7e0088, &physics::GroundConstants::overspeed_alignment},
    {0x7e00e4, &physics::GroundConstants::steep_negative_alignment},
    {0x7e0124, &physics::GroundConstants::shallow_slope_alignment},
    {0x7e0138, &physics::GroundConstants::slide_alignment},
    {0x7e0148, &physics::GroundConstants::stop_minimum},
    {0x7e014c, &physics::GroundConstants::stop_maximum},
    {0x7e0178, &physics::GroundConstants::slide_speed},
    {0x7e019c, &physics::GroundConstants::overspeed_drag_multiplier},
    {0x7e01a0, &physics::GroundConstants::attribute_alignment},
    {0x7e01a4, &physics::GroundConstants::side_slope_multiplier},
    {0x7e01a8, &physics::GroundConstants::medium_negative_alignment},
};
struct ReferenceStop {};
struct Binding {
    PlayerTaskState &task;
    PlayerMotionState &world;
    PlayerPhysicsState &player;
    const PlayerParameters &parameters;
    const PlayerAccelerationRuntime &runtime;
    const GroundAccelerationCallees &callees;
    physics::GroundConstants constants;
    PlayerAccelerationResult result{};
    bool gravity_formed = false;
    std::optional<std::int32_t> angle{};
    std::optional<float> magnitude{};
};
thread_local Binding *active = nullptr;
Binding &binding() {
    if (!active)
        throw std::logic_error("ground reference binding is absent");
    return *active;
}
struct BindScope {
    Binding *previous;
    explicit BindScope(Binding &value) : previous(active) { active = &value; }
    ~BindScope() { active = previous; }
};
[[noreturn]] void stop(physics::GroundContext &context, PlayerAccelerationFailure failure,
                       std::uint32_t site, std::uint32_t callee = 0) {
    auto &current = binding();
    current.result = {failure, site, callee,
                      current.gravity_formed ? std::optional<NinjaVector>(context.acceleration)
                                             : std::nullopt};
    throw ReferenceStop{};
}
void mutate(physics::GroundContext &context, const GroundAccelerationMutation &fn,
            std::uint32_t site, std::uint32_t target) {
    auto &current = binding();
    if (!fn)
        stop(context, PlayerAccelerationFailure::missing_callee, site, target);
    const auto result =
        fn(current.task, current.world, current.player, current.parameters, context.fp);
    if (!result) {
        current.result = result;
        current.result.callee_failure = result.failure;
        current.result.failure = PlayerAccelerationFailure::callee_unavailable;
        current.result.source_boundary = site;
        current.result.callee_address = target;
        current.result.nested_source_boundary = result.source_boundary;
        throw ReferenceStop{};
    }
}
} // namespace
float GroundAccelerationConstants::at(std::uint32_t address) const {
    for (std::size_t index = 0; index < std::size(constant_fields); ++index) {
        if (constant_fields[index].address == address) {
            float result;
            std::memcpy(&result, &bits[index], sizeof(result));
            return result;
        }
    }
    throw std::logic_error("Unbound ground-acceleration source constant address");
}
physics::GroundConstants decode_ground_constants(const GroundAccelerationConstants &source) {
    physics::GroundConstants result{};
    for (std::size_t index = 0; index < std::size(constant_fields); ++index)
        std::memcpy(&(result.*constant_fields[index].member), &source.bits[index], sizeof(float));
    return result;
}
std::array<std::uint32_t, 24> native_ground_constant_words() {
    const physics::GroundConstants values{};
    std::array<std::uint32_t, 24> result{};
    for (std::size_t index = 0; index < std::size(constant_fields); ++index)
        std::memcpy(&result[index], &(values.*constant_fields[index].member), sizeof(float));
    return result;
}
PlayerAccelerationResult
get_player_acceleration_ground(PlayerTaskState &task, PlayerMotionState &world,
                               PlayerPhysicsState &player, const PlayerParameters &parameters,
                               const GroundAccelerationConstants &constants,
                               const PlayerAccelerationRuntime &runtime,
                               const GroundAccelerationCallees &callees, X87Context &fp) {
    Binding current{
        task, world, player, parameters, runtime, callees, decode_ground_constants(constants),
        {},   false, {},     {}};
    BindScope scope(current);
    physics::GroundMotion motion{
        task.orientation,      world.motion,          player.motion,
        player.dotp,           player.attributes,     world.target_yaw,
        world.source_field_34, world.source_field_38, world.friction_scale};
    const physics::GroundEnvironment environment{};
    try {
        physics::calculate_ground_acceleration(motion, parameters, environment, fp);
    } catch (const ReferenceStop &) {
    }
    return current.result;
}
namespace physics {
const GroundConstants &ground_constants(GroundContext &) { return binding().constants; }
const NinjaVector &read_world_gravity(GroundContext &context) {
    if (!binding().runtime.world_gravity)
        stop(context, PlayerAccelerationFailure::unavailable_gravity, 0x44c2e0);
    return *binding().runtime.world_gravity;
}
void report_ground_acceleration(GroundContext &context) {
    binding().gravity_formed = true;
    binding().result.temporary_acceleration = context.acceleration;
}
void convert_gravity_to_player(GroundContext &context) {
    auto &current = binding();
    if (!current.callees.world_to_player)
        stop(context, PlayerAccelerationFailure::missing_world_to_player, 0x44c357, 0x43ec00);
    const auto result =
        current.callees.world_to_player(current.task.orientation, context.acceleration, context.fp);
    if (!result) {
        try {
            stop(context, PlayerAccelerationFailure::coordinate_unavailable, 0x44c357, 0x43ec00);
        } catch (const ReferenceStop &) {
            current.result.coordinate_failure = result.failure;
            current.result.nested_source_boundary = result.source_boundary;
            throw;
        }
    }
}
bool query_ground_input(GroundContext &context) {
    auto &current = binding();
    std::int32_t angle;
    float magnitude;
    const auto result =
        get_player_input(input_task_view(current.task), current.runtime.input, context.fp, &angle,
                         &magnitude, current.runtime.forced_angle);
    if (!result || !result.active) {
        try {
            stop(context, PlayerAccelerationFailure::input_unavailable, 0x44c367, 0x43bdc0);
        } catch (const ReferenceStop &) {
            current.result.input_failure = result.failure;
            current.result.nested_source_boundary = result.source_boundary;
            throw;
        }
    }
    if ((result.outputs_written & 1U) != 0)
        current.angle = angle;
    if ((result.outputs_written & 2U) != 0)
        current.magnitude = magnitude;
    return *result.active;
}
std::int32_t ground_input_angle(GroundContext &context) {
    if (!binding().angle)
        stop(context, PlayerAccelerationFailure::input_outputs_unavailable, 0x44c6d3, 0x43bdc0);
    return *binding().angle;
}
float ground_input_magnitude(GroundContext &context, InputGainBand band) {
    if (!binding().magnitude) {
        std::uint32_t site = 0;
        switch (band) {
        case InputGainBand::Jog:
            site = 0x44c6fb;
            break;
        case InputGainBand::Run:
            site = 0x44c72a;
            break;
        case InputGainBand::Fast:
            site = 0x44c781;
            break;
        case InputGainBand::AboveMaximumNegative:
            site = 0x44c7c8;
            break;
        case InputGainBand::AboveMaximumOther:
            site = 0x44c7d4;
            break;
        }
        stop(context, PlayerAccelerationFailure::input_outputs_unavailable, site, 0x43bdc0);
    }
    return *binding().magnitude;
}
NinjaVector ground_contact_direction(GroundContext &context) {
    const auto direction = player_contact_direction(binding().player);
    if (!direction)
        stop(context, PlayerAccelerationFailure::unavailable_collision_vector, 0x44c36c, 0x7889f0);
    return *direction;
}
void event_ground_acceleration(GroundContext &context) {
    mutate(context, binding().callees.event_acceleration, 0x44c293, 0x447510);
}
void attribute_ground_acceleration(GroundContext &context) {
    mutate(context, binding().callees.attribute_20000_acceleration, 0x44c2b8, 0x44b0e0);
}
void alternate_ground_acceleration(GroundContext &context) {
    mutate(context, binding().callees.attribute_4000_acceleration, 0x44c2d2, 0x447010);
}
void passive_ground_rotation(GroundContext &context) {
    mutate(context, binding().callees.passive_rotation, 0x44c620, 0x4491e0);
}
void reset_ground_direction(GroundContext &context) {
    mutate(context, binding().callees.direction_reset, 0x44c691, 0x43c130);
}
void rotate_ground_attribute(GroundContext &context) {
    mutate(context, binding().callees.attribute_rotation, 0x44c6c5, 0x449380);
}
void turn_ground_yaw(GroundContext &context, std::int32_t angle, GroundYaw yaw) {
    auto &current = binding();
    const GroundYawMutation *fn = nullptr;
    std::uint32_t site = 0, target = 0;
    switch (yaw) {
    case GroundYaw::Fast:
        fn = &current.callees.yaw_fast;
        site = 0x44c81e;
        target = 0x443df0;
        break;
    case GroundYaw::BlendedSmallTurn:
        fn = &current.callees.yaw_blended;
        site = 0x44c889;
        target = 0x443e60;
        break;
    case GroundYaw::BlendedAttribute:
        fn = &current.callees.yaw_blended;
        site = 0x44c8b8;
        target = 0x443e60;
        break;
    case GroundYaw::Regular:
        fn = &current.callees.yaw_regular;
        site = 0x44c904;
        target = 0x443c50;
        break;
    }
    if (!*fn)
        stop(context, PlayerAccelerationFailure::missing_callee, site, target);
    const auto result =
        (*fn)(current.task, current.world, current.player, current.parameters, angle, context.fp);
    if (!result) {
        current.result = result;
        current.result.callee_failure = result.failure;
        current.result.failure = PlayerAccelerationFailure::callee_unavailable;
        current.result.source_boundary = site;
        current.result.callee_address = target;
        current.result.nested_source_boundary = result.source_boundary;
        throw ReferenceStop{};
    }
}
} // namespace physics
} // namespace sadx
