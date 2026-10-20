#include "player/player_heading.hpp"
#include "player/player_rotation.hpp"
#include "player/player_scene_position_internal.hpp"
#include <utility>

namespace sadx::scene {
using fidelity::ExactFloat;
// NOTSA: named retained-value interfaces to the existing Ninja primitives.
ExactFloat Context::length(const NinjaVector &value) {
    return ExactFloat::retained(fp, rotation_vector_length(value, fp));
}
ExactFloat Context::dot(const NinjaVector &a, const NinjaVector &b) {
    return ExactFloat::retained(fp, inner_product_extended(a, b, fp));
}
ExactFloat Context::cross(const NinjaVector &a, const NinjaVector &b, NinjaVector &out) {
    return ExactFloat::retained(fp, cross_product_length_extended(a, b, out, fp));
}
ExactFloat Context::signed_sqrt(float value) {
    return ExactFloat::retained(fp, signed_sqrt_extended(value, fp));
}
namespace {
// NOTSA: preserve wrapped signed angle words without implementation-defined casts.
std::int32_t signed_bits(std::uint32_t value) {
    std::int32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
// NOTSA: source carry and parity tests include their explicit unordered cases.
bool less_or_unordered(X87Comparison value) {
    return value == X87Comparison::less || value == X87Comparison::unordered;
}
bool greater_or_unordered(X87Comparison value) {
    return value == X87Comparison::greater || value == X87Comparison::unordered;
}
} // namespace
void Context::subtract_angle(std::int32_t value) {
    orientation.angles[1] = signed_bits(static_cast<std::uint32_t>(orientation.angles[1]) -
                                        static_cast<std::uint32_t>(value));
}
void Context::align_motion_yaw(Access calculation) {
    auto &yaw = registered_yaw(calculation);
    if (angle_distance(yaw, orientation.angles[1]) > 24576U)
        orientation.angles[1] =
            signed_bits(static_cast<std::uint32_t>(orientation.angles[1]) + 32768U);
    yaw = orientation.angles[1];
}
void Context::scale_retained(NinjaVector &out, const NinjaVector &direction, ExactFloat scale) {
    out.x = (exact(direction.x) * scale).rounded();
    out.y = (exact(direction.y) * scale).rounded();
    out.z = (scale * direction.z).rounded();
}
namespace {
// Slide along the intersection of the contact plane and the movement plane.
// FUNCTION: SONIC 0x43dca0 SlideSceneWall
void slide_scene_wall(Context &o, const NinjaVector &input) {
    if ((o.status(Access::WallSlideSpeedGateStatus) & 3U) == 0 &&
        !greater_or_unordered((o.length(o.motion.speed)).compare(o.parameters.jog_speed)))
        return;
    NinjaVector axis;
    const auto cross = o.cross(input, o.surface_normal(Access::WallSlideCrossSurfaceNormal), axis);
    if (cross.compare(0.0f) == X87Comparison::equal)
        return;
    if (greater_or_unordered((o.length(axis)).compare(o.tuning.wall_axis_minimum)))
        o.normalize(axis);
    else
        axis = o.direction(Access::WallSlideFallbackDirection);
    const NinjaPlane first{o.response_plane(Access::WallSlidePlanePoint),
                           o.surface_normal(Access::WallSlidePlaneNormal)};
    const NinjaPlane second{o.center(Access::WallSlidePlaneCenter), axis};
    NinjaPlane intersection;
    o.intersect_planes(first, second, intersection);
    auto normalized = input;
    if ((o.length(normalized)).compare(0.0f) != X87Comparison::equal)
        o.normalize(normalized);
    if ((o.dot(intersection.normal, normalized)).compare(0.0f) == X87Comparison::greater) {
        intersection.normal.x = ((-o.exact(intersection.normal.x))).rounded();
        intersection.normal.y = ((-o.exact(intersection.normal.y))).rounded();
        intersection.normal.z = ((-o.exact(intersection.normal.z))).rounded();
    }
    auto &horizontal = o.slide_vector(Access::WallSlideVelocitySlideVector);
    const auto scale = o.length((o.status(Access::Known) & 3U)
                                    ? o.displacement(Access::WallSlideGroundDisplacement)
                                    : horizontal);
    o.scale_retained(horizontal, intersection.normal, scale);
    if (o.prior_attributes(Access::WallSlideRegisteredVelocityPriorAttributes) & 8388608U) {
        auto &registered_speed = o.registered_velocity(Access::WallSlideRegisteredVelocityRegisteredVelocity);
        const auto speed = o.length(registered_speed);
        o.scale_retained(registered_speed, intersection.normal, speed);
    }
    auto old = o.displacement(Access::WallSlideTurnDisplacement);
    if ((o.length(old)).compare(o.tuning.turn_direction_minimum) == X87Comparison::greater) {
        normalized = horizontal;
        if ((o.length(normalized)).compare(o.tuning.turn_direction_minimum) ==
            X87Comparison::greater) {
            o.normalize(old);
            o.normalize(normalized);
            o.subtract_angle(o.angle(old, normalized, &o.direction(Access::WallSlideTurnDirection),
                                     Access::WallSlideTurnAngle));
        }
    }
    o.align_motion_yaw(Access::WallSlideTurnRegisteredYaw);
}
// Corner sliding turns the player before adjusting the registered speed.
// FUNCTION: SONIC 0x43df60 SlideSceneCorner
void slide_scene_corner(Context &o, const NinjaVector &input) {
    if ((o.status(Access::CornerSlideStatus) & 3U) == 0 &&
        !greater_or_unordered((o.length(o.motion.speed)).compare(o.parameters.jog_speed)))
        return;
    NinjaVector cross;
    if ((o.cross(input, o.surface_normal(Access::CornerSlideSurfaceNormal), cross))
            .compare(0.0f) == X87Comparison::equal)
        return;
    auto old = o.displacement(Access::CornerSlideDisplacement),
         next = o.slide_vector(Access::CornerSlideSlideVector);
    if ((o.length(next)).compare(0.0f) == X87Comparison::equal)
        return;
    o.normalize(old);
    o.normalize(next);
    o.subtract_angle(o.angle(old, next, &o.direction(Access::CornerSlideTurnDirection),
                             Access::CornerSlideTurnAngle));
    o.align_motion_yaw(Access::CornerSlideTargetYaw);
    next = o.slide_vector(Access::Known);
    if (!less_or_unordered((o.length(next)).compare(o.tuning.corner_speed_minimum))) {
        auto &registered_speed = o.registered_velocity(Access::CornerSlideRegisteredVelocity);
        const auto retained = o.length(registered_speed);
        float speed = retained.rounded();
        if (retained.compare(o.tuning.corner_fast_speed) == X87Comparison::greater)
            speed = ((o.exact(speed) * o.tuning.corner_speed_multiplier)).rounded();
        o.normalize(next);
        registered_speed.x = ((o.exact(next.x) * speed)).rounded();
        registered_speed.y = ((o.exact(next.y) * speed)).rounded();
        registered_speed.z = ((o.exact(next.z) * speed)).rounded();
    }
}
// NOTSA: named phase shared by the recovered scene dispatchers.
NinjaVector negated_world(Context &o) {
    const auto &speed = o.world_velocity(Access::WallWorldDirectionWorld);
    return {((-o.exact(speed.x))).rounded(), ((-o.exact(speed.y))).rounded(),
            ((-o.exact(speed.z))).rounded()};
}
// NOTSA: named phase shared by the recovered scene dispatchers.
NinjaVector old_position_direction(Context &o, const NinjaVector &old) {
    const auto &current = o.position(Access::WallPositionDirectionPosition);
    return {((o.exact(old.x) - current.x)).rounded(), ((o.exact(old.y) - current.y)).rounded(),
            ((o.exact(old.z) - current.z)).rounded()};
}
// NOTSA: named phase shared by the recovered scene dispatchers.
void commit_wall_position(Context &o, const NinjaVector &offset) {
    o.center(o.corrected_plane(Access::WallPositionCommitCorrectedPlane));
    auto &position = o.position(Access::WallPositionCommitPosition);
    const auto &center = o.center(Access::Known);
    position.x = ((o.exact(center.x) - offset.x)).rounded();
    position.y = ((o.exact(center.y) - offset.y)).rounded();
    position.z = ((o.exact(center.z) - offset.z)).rounded();
    o.displacement(o.slide_vector(Access::WallPositionCommitSlideVector));
}
// FUNCTION: SONIC 0x444aa0 RespondSceneWall
std::int32_t wall_scene_response(Context &o, const NinjaVector &old, const NinjaVector &offset,
                                 std::int32_t accumulator, std::uint32_t attributes) {
    o.collision_normal(o.surface_normal(Access::WallResponseSurfaceNormal));
    NinjaVector direction;
    bool slide = false;
    if (o.orientation.flags & 1U) {
        direction = old_position_direction(o, old);
        if ((o.length(direction)).compare(0.0f) == X87Comparison::equal) {
            commit_wall_position(o, offset);
            return accumulator;
        }
        o.normalize(direction);
        const auto angle = o.angle(direction, o.surface_normal(Access::Known), nullptr,
                                   Access::GroundedWallResponseAngle);
        if (attributes & 2097152U) {
            if (angle > 16384)
                return accumulator;
            accumulator = 1;
            slide = true;
        } else if (!less_or_unordered(o.compare(o.motion.speed.x, o.tuning.wall_stop_speed))) {
            o.slide_vector(NinjaVector{0, 0, 0});
            accumulator = 2;
            o.motion.speed = {0, 0, 0};
        } else if (angle <= 4096) {
            if (!less_or_unordered(o.compare(o.motion.speed.x, o.parameters.crash_speed)))
                accumulator = 2;
            else {
                accumulator = 1;
                slide = true;
            }
        } else if (angle <= 16384) {
            accumulator = 1;
            slide = true;
        }
    } else {
        if (o.motion_override(Access::WallResponseMotionOverride)) {
            direction = {-1, 0, 0};
            o.task_to_world(direction, Access::WallResponseTaskToWorld);
        } else
            direction = negated_world(o);
        if ((o.length(direction)).compare(0.0f) != X87Comparison::equal) {
            o.normalize(direction);
            const auto angle = o.angle(direction, o.surface_normal(Access::Known), nullptr,
                                       Access::AirborneWallResponseAngle);
            const auto dot = o.dot(direction, o.surface_normal(Access::Known));
            if (attributes & 2097152U) {
                if (angle > 16384)
                    return accumulator;
                accumulator = 1;
                slide = true;
            } else {
                const bool slow = less_or_unordered(o.compare(o.motion.speed.x, o.tuning.wall_stop_speed));
                const auto dot_cmp = dot.compare(o.tuning.wall_alignment);
                if (!slow && !less_or_unordered(dot_cmp)) {
                    o.slide_vector(NinjaVector{0, 0, 0});
                    accumulator = 2;
                    o.motion.speed = {0, 0, 0};
                } else if (slow && !less_or_unordered(dot_cmp)) {
                    if (!less_or_unordered(o.compare(o.motion.speed.x, o.parameters.crash_speed)))
                        accumulator = 2;
                    else
                        accumulator = 1;
                } else if (o.character_id == 3 && angle <= 13312 && !(attributes & 2101248U))
                    accumulator = 1;
                else if (angle >= 16384)
                    return accumulator;
                else
                    slide = true;
            }
        }
    }
    if (slide)
        slide_scene_wall(o, direction);
    commit_wall_position(o, offset);
    return accumulator;
}
// FUNCTION: SONIC 0x444d50 RespondSceneCorner
std::int32_t corner_scene_response(Context &o, const NinjaVector &old, const NinjaVector &offset,
                                   std::int32_t accumulator, std::uint32_t attributes) {
    o.collision_normal(o.surface_normal(Access::CornerResponseSurfaceNormal));
    NinjaVector direction;
    if (o.orientation.flags & 1U) {
        direction = old_position_direction(o, old);
        if ((o.length(direction)).compare(0.0f) == X87Comparison::equal) {
            direction = negated_world(o);
            if ((o.length(direction)).compare(0.0f) == X87Comparison::equal)
                return accumulator;
        }
        o.normalize(direction);
        if (o.angle(direction, o.surface_normal(Access::Known), nullptr,
                    Access::GroundedCornerResponseAngle) >= 16384)
            return accumulator;
        accumulator = 1;
    } else {
        direction = negated_world(o);
        if ((o.length(direction)).compare(0.0f) == X87Comparison::equal) {
            commit_wall_position(o, offset);
            return accumulator;
        }
        o.normalize(direction);
        const auto angle = o.angle(direction, o.surface_normal(Access::Known), nullptr,
                                   Access::AirborneCornerResponseAngle);
        (void)o.dot(direction, o.surface_normal(Access::Known));
        if (attributes & 2097152U) {
            if (angle >= 16384)
                return accumulator;
            accumulator = 1;
        } else if (angle >= 16384)
            return accumulator;
    }
    slide_scene_corner(o, direction);
    commit_wall_position(o, offset);
    return accumulator;
}

// NOTSA: named floor-coordinate step with the caller's diagnostic calculation.
void floor_transform(Context &o, const NinjaVector &normal, NinjaVector &value,
                     Access calculation) {
    o.floor_to_world(normal, value, calculation);
}

// NOTSA: named phase shared by the recovered scene dispatchers.
void contact_offsets(Context &o, bool alternate, bool use_floor, const NinjaVector *floor,
                     std::optional<float> initial_half = {}) {
    NinjaVector half{0, 0, 0}, center{0, 0, 0};
    if (alternate) {
        o.coordinates.begin_unit_frame();
        auto &matrix = o.coordinates.current_matrix();
        if (o.orientation.angles[2])
            o.coordinates.rotate_z(matrix, static_cast<std::uint16_t>(o.orientation.angles[2]),
                                   o.fp);
        if (o.orientation.angles[0])
            o.coordinates.rotate_x(matrix, static_cast<std::uint16_t>(o.orientation.angles[0]),
                                   o.fp);
        if (o.orientation.angles[1])
            o.coordinates.rotate_y(matrix,
                                   static_cast<std::uint16_t>(
                                       0U - static_cast<std::uint32_t>(o.orientation.angles[1])),
                                   o.fp);
        o.contact_pitch(o.orientation.angles[0]);
        o.contact_roll(o.orientation.angles[2]);
        const NinjaVector vertical{0, 1, 0};
        NinjaVector direction;
        o.calculate_vector(matrix, vertical, direction, Access::ContactOffsetDirectionCalculateVector);
        o.direction(direction);
        half.y = ((o.exact(o.parameters.height) * 0.5f)).rounded();
        center = half;
        NinjaVector transformed_half;
        o.calculate_vector(matrix, half, transformed_half,
                           Access::ContactOffsetHalfHeightCalculateVector);
        half = transformed_half;
        o.initialize_offset(center, ActorSelection::Passed, Access::AlternateContactOffsetActorInitializeOffset);
        NinjaVector transformed_center;
        o.calculate_vector(matrix, center, transformed_center,
                           Access::ContactOffsetCenterCalculateVector);
        center = transformed_center;
        o.coordinates.end_frame();
    } else {
        if (!initial_half) {
            o.direction(NinjaVector{0, 1, 0});
            o.contact_pitch(o.orientation.angles[0]);
            o.contact_roll(o.orientation.angles[2]);
        }
        half.y = initial_half ? *initial_half
                              : ((o.exact(o.parameters.height) * 0.5f)).rounded();
        center = half;
        if (use_floor) {
            floor_transform(o, *floor, o.direction(Access::Known),
                            Access::ContactOffsetDirectionFloorToWorld);
            floor_transform(o, *floor, half, Access::ContactOffsetHalfHeightFloorToWorld);
        } else {
            o.task_to_world(o.direction(Access::Known),
                            Access::ContactOffsetDirectionTaskToWorld);
            o.task_to_world(half, Access::ContactOffsetHalfHeightTaskToWorld);
        }
        o.initialize_offset(center, ActorSelection::Passed,
                            use_floor ? Access::ContactOffsetFloorInitializeOffset
                                      : Access::OrdinaryContactOffsetActorInitializeOffset);
        if (use_floor)
            floor_transform(o, *floor, center, Access::ContactOffsetCenterFloorToWorld);
        else
            o.task_to_world(center, Access::ContactOffsetCenterTaskToWorld);
    }
    o.remember_offsets(half, center);
}
// NOTSA: named phase shared by the recovered scene dispatchers.
void reset_sweep(Context &o, Access site) {
    auto &center = o.center(site);
    const auto &offset = o.result.center_offset;
    const auto &position = o.position(site);
    center.x = ((o.exact(offset.x) + position.x)).rounded();
    center.y = ((o.exact(offset.y) + position.y)).rounded();
    center.z = ((o.exact(offset.z) + position.z)).rounded();
    o.displacement(o.world_velocity(site));
}
// NOTSA: named phase shared by the recovered scene dispatchers.
void add_displacement(Context &o, bool direct_world = false) {
    auto &position = o.position(Access::MovePositionPosition);
    const auto &displacement =
        direct_world ? o.world_velocity(Access::MovePositionWorld)
                     : o.displacement(Access::MovePositionDisplacement);
    position.x = ((o.exact(position.x) + displacement.x)).rounded();
    position.y = ((o.exact(displacement.y) + position.y)).rounded();
    position.z = ((o.exact(displacement.z) + position.z)).rounded();
}
// NOTSA: first-pass admission shared by the two recovered dispatchers.
bool entry_allowed(Context &o, EntrySlot entry, bool alternate) {
    const auto attributes = o.entry_attributes(
        entry, alternate ? Access::AlternateCandidateAdmissionEntryAttributes
                         : Access::OrdinaryCandidateAdmissionEntryAttributes);
    if ((attributes & 4194306U) && o.character_id != 6)
        return false;
    const auto task_ref =
        o.entry_task(entry, alternate ? Access::AlternateCandidateAdmissionEntryTask
                                      : Access::OrdinaryCandidateAdmissionEntryTask);
    if (!task_ref.id)
        return true;
    const auto task = o.geometry_task(
        task_ref, alternate ? Access::AlternateCandidateAdmissionGeometryTask
                            : Access::OrdinaryCandidateAdmissionGeometryTask);
    if (!o.geometry_has_primary(task)) {
        if (alternate)
            o.fail(Failure::NullGeometryPrimary,
                   Access::CandidateAdmissionNullGeometryPrimary);
        return true;
    }
    return (o.geometry_flags(
                task, alternate
                          ? Access::CandidateAdmissionNullGeometryPrimary
                          : Access::CandidateAdmissionGeometryFlags) &
            256U) != 0;
}

// NOTSA: named phase shared by the recovered scene dispatchers.
void first_scene_pass(Context &o, bool alternate) {
    const auto capacity = alternate ? 32U : 64U;
    for (std::int32_t index = 0; index < o.candidate_count(); ++index) {
        o.result.candidate_index = static_cast<std::size_t>(index);
        const auto entry =
            o.entry(EntryCollection::Candidates, static_cast<std::size_t>(index),
                    alternate ? Access::AlternateCandidateAdmissionEntryAttributes
                              : Access::OrdinaryCandidateAdmissionEntryAttributes);
        if (!entry_allowed(o, entry, alternate))
            continue;
        const auto attributes = o.entry_attributes(entry, Access::Known);
        if (o.query(entry,
                    alternate ? Access::AlternateFirstPassQueryQuery : Access::OrdinaryFirstPassQueryQuery)) {
            const auto grounded = (o.status(alternate ? Access::AlternateFirstPassQueryStatus
                                                      : Access::OrdinaryFirstPassQueryStatus) &
                                   3U) != 0;
            const auto source =
                grounded ? (alternate ? Response::AlternateGrounded : Response::OrdinaryGrounded)
                         : Response::Airborne;
            const auto response =
                o.respond(source, attributes,
                          grounded ? (alternate ? Access::AlternateFirstPassGroundedResponseResponse
                                                : Access::OrdinaryFirstPassGroundedResponseResponse)
                                   : (alternate ? Access::AlternateFirstPassAirborneResponseResponse
                                                : Access::OrdinaryFirstPassAirborneResponseResponse));
            if (response == 1)
                o.result.winning_index = o.result.processed_count;
        }
        if (o.result.processed_count >= capacity)
            o.fail(Failure::LocalBufferOverflow, alternate ? Access::AlternateFirstPassCapacityLocalBufferOverflow
                                                           : Access::OrdinaryFirstPassCapacityLocalBufferOverflow);
        o.copy_entry(entry, {EntryCollection::FirstPass, o.result.processed_count++});
        if (!alternate && (attributes & 16384U))
            o.special_surface(1);
    }
}
// NOTSA: named phase shared by the recovered scene dispatchers.
void order_scene_pass(Context &o, bool alternate) {
    const auto count = o.result.processed_count;
    std::size_t cursor = 0;
    if (o.result.winning_index)
        o.copy_entry({EntryCollection::FirstPass, *o.result.winning_index},
                     {EntryCollection::Ordered, cursor++});
    const bool forward = static_cast<bool>(o.moving_task(
        alternate ? Access::AlternateCandidateOrderMovingTask : Access::OrdinaryCandidateOrderMovingTask));
    for (std::size_t ordinal = 0; ordinal < count; ++ordinal) {
        const auto index = forward ? ordinal : count - 1 - ordinal;
        if (o.result.winning_index && index == *o.result.winning_index)
            continue;
        const auto entry =
            o.entry(EntryCollection::FirstPass, index,
                    alternate ? Access::AlternateCandidateOrderEntry : Access::OrdinaryCandidateOrderEntry);
        if (o.cached_object(alternate ? Access::AlternateCandidateOrderCachedObject
                                      : Access::OrdinaryCandidateOrderCachedObject)
                .id == o.entry_object(entry, alternate ? Access::AlternateCandidateOrderCachedObject
                                                       : Access::OrdinaryCandidateOrderCachedObject)
                           .id)
            o.copy_entry(entry, {EntryCollection::Ordered, count - 1});
        else {
            if (cursor >= count)
                o.fail(Failure::LocalBufferOverflow, alternate
                                                         ? Access::AlternateCandidateOrderLocalBufferOverflow
                                                         : Access::OrdinaryCandidateOrderLocalBufferOverflow);
            o.copy_entry(entry, {EntryCollection::Ordered, cursor++});
        }
    }
}
// NOTSA: named phase shared by the recovered scene dispatchers.
void airborne_scene_tail(Context &o, bool alternate) {
    bool back = false;
    const auto flags = o.status(alternate ? Access::AlternateAirborneTailStatus
                                          : Access::OrdinaryAirborneTailStatus);
    if ((flags & 1U) && !(flags & 16384U)) {
        const auto input = o.read_input(alternate ? Access::AlternateAirborneTailInput
                                                  : Access::OrdinaryAirborneTailInput);
        const float magnitude = input.magnitude;
        bool skip = false;
        if (input.active) {
            skip = greater_or_unordered(o.compare(magnitude, o.tuning.active_input_minimum));
            if (!skip)
                skip =
                    o.no_control_time(alternate ? Access::AlternateAirborneTailNoControlTime
                                                : Access::OrdinaryAirborneTailNoControlTime) != 0;
        }
        if (!skip && !greater_or_unordered(o.compare(o.motion.speed.y, 0.0f)) &&
            !greater_or_unordered(o.compare(o.motion.speed.x, o.parameters.run_speed))) {
            if ((alternate ||
                 !(o.prior_attributes(Access::AirborneTailPriorAttributes) & 28U)) &&
                o.compare(o.alignment, o.tuning.backstep_alignment) == X87Comparison::greater)
                back = true;
        }
    }
    if (back) {
        auto &position = o.position(alternate ? Access::AlternateAirborneTailPosition
                                              : Access::OrdinaryAirborneTailPosition);
        auto &speed =
            o.world_velocity(alternate ? Access::AlternateAirborneTailWorld
                                       : Access::OrdinaryAirborneTailWorld);
        position.x = ((position.x - (o.exact(speed.x) * 0.5f))).rounded();
        position.y = ((position.y - (o.exact(speed.y) * 0.5f))).rounded();
        position.z = ((position.z - (o.exact(speed.z) * 0.5f))).rounded();
        speed = {0, 0, 0};
    } else
        add_displacement(o);
    o.attributes(0, alternate ? Access::AlternateAirborneTailAttributes
                              : Access::OrdinaryAirborneTailAttributes);
}
// FUNCTION: SONIC 0x444640 AcceptSceneContact
bool scene_contact_gate(Context &o, EntrySlot entry) {
    o.save_contact();
    o.select_registered_player();
    const auto &p = o.registered_parameters();
    const auto special = o.special_surface(Access::ContactGateCenterSpecialSurface);
    NinjaVector center{0, 4, 0};
    if (special && o.player_index == 5)
        center.y = o.amy_center_offset(Access::ContactGateCenterAmyCenterOffset);
    o.initialize_offset(center, ActorSelection::Registered, Access::ContactGateCenterInitializeOffset);
    const bool floor = special && !(o.orientation.flags & 8192U);
    if (floor)
        floor_transform(o, o.surface_normal(Access::ContactGateCenterSurfaceNormal), center,
                        Access::ContactGateCenterFloorToWorld);
    else
        o.task_to_world(center, Access::ContactGateCenterTaskToWorld);
    NinjaVector radius_offset{0, o.radius(Access::ContactGateRadiusRadius), 0};
    if (special && o.player_index == 5)
        radius_offset.y = o.amy_radius_offset(Access::ContactGateRadiusAmyRadiusOffset);
    if (floor)
        floor_transform(o, o.surface_normal(Access::Known), radius_offset,
                        Access::ContactGateRadiusFloorToWorld);
    else
        o.task_to_world(radius_offset, Access::ContactGateRadiusTaskToWorld);
    const auto attributes = o.entry_attributes(entry, Access::ContactGateSweepEntryAttributes);
    auto &point = o.center(Access::ContactGateSweepCenter);
    const auto add_offset = [&](const NinjaVector &base) {
        point.x = ((o.exact(center.x) + base.x)).rounded();
        point.y = ((o.exact(center.y) + base.y)).rounded();
        point.z = ((o.exact(center.z) + base.z)).rounded();
    };
    if (floor) {
        const auto &plane = o.corrected_plane(Access::ContactGateSweepCorrectedPlane);
        if (attributes & 16384U) {
            o.displacement(NinjaVector{0, 0, 0});
            const auto &horizontal = o.slide_vector(Access::ContactGateSweepSlideVector);
            point.x =
                ((((o.exact(horizontal.x) + plane.x) - radius_offset.x) + center.x)).rounded();
            point.y =
                ((((o.exact(horizontal.y) + plane.y) - radius_offset.y) + center.y)).rounded();
            point.z =
                ((((o.exact(horizontal.z) + plane.z) - radius_offset.z) + center.z)).rounded();
        } else {
            point.x = (((o.exact(plane.x) - radius_offset.x) + center.x)).rounded();
            point.y = (((o.exact(plane.y) - radius_offset.y) + center.y)).rounded();
            point.z = (((o.exact(plane.z) - radius_offset.z) + center.z)).rounded();
            if (o.angle(o.direction(Access::ContactGateSlopeDirection),
                        o.surface_normal(Access::ContactGateSlopeSurfaceNormal), nullptr,
                        Access::ContactGateSlopeAngle) >= 11264) {
                auto horizontal = o.slide_vector(Access::ContactGateSlopeSlideVector);
                if (!less_or_unordered((o.length(horizontal)).compare(1.0f))) {
                    o.normalize(horizontal);
                    o.scale_retained(o.displacement(Access::ContactGateSlopeDisplacement),
                                     horizontal, o.length(o.displacement(Access::Known)));
                }
            }
        }
    } else if (o.player_index == 7)
        add_offset(o.response_plane(Access::ContactGateUnalignedCenterResponsePlane));
    else {
        const auto old = point;
        const auto &plane = o.corrected_plane(Access::ContactGateUnalignedCenterCorrectedPlane);
        const auto &position = o.position(Access::ContactGateUnalignedCenterPosition);
        point.x = ((((o.exact(plane.x) - old.x) + position.x) + center.x)).rounded();
        point.y = ((((o.exact(plane.y) - old.y) + position.y) + center.y)).rounded();
        point.z = ((((o.exact(plane.z) - old.z) + position.z) + center.z)).rounded();
    }
    const auto high = o.compare(o.registered_player_speed().x, p.dash_speed);
    if (!less_or_unordered(high) ||
        (o.registered_attributes(Access::ContactGateRadiusRegisteredAttributes) & 16384U) ||
        (!greater_or_unordered(o.compare(o.registered_player_speed().y, 0.0f)) &&
         (attributes & 16384U)))
        o.radius(5.1F);
    else if (less_or_unordered(o.compare(o.registered_player_speed().x, p.jog_speed)))
        o.radius(4.1F);
    else {
        const auto speed = (o.exact(o.registered_player_speed().x) - p.jog_speed);
        const auto range = (o.exact(p.dash_speed) - p.jog_speed);
        o.radius((((speed / range) + o.tuning.horizontal_contact_limit)).rounded());
    }
    if (o.player_index == 7)
        o.radius(
            ((o.exact(o.radius(Access::Known)) + o.tuning.big_radius_addition)).rounded());
    if (o.render_mode(Access::ContactGateRadiusRenderMode) >= 0) {
        o.coordinates.push_current_frame();
        o.coordinates.translate_matrix(o.coordinates.current_matrix(), point, o.fp);
        o.draw_contact_marker();
    }
    if (o.query(entry, Access::ContactGateQueryQuery)) {
        const auto normal_angle =
            o.angle(o.direction(Access::ContactGateQueryDirection),
                    o.surface_normal(Access::ContactGateQuerySurfaceNormal), nullptr,
                    Access::ContactGateQueryAngle);
        bool accept = true;
        if (normal_angle > 11264) {
            const auto &gravity = o.gravity(Access::ContactGateQueryGravity);
            const float old_dot = (o.dot(gravity, o.direction(Access::Known))).rounded();
            const auto comparison =
                (o.dot(gravity, o.surface_normal(Access::Known))).compare(old_dot);
            if (comparison != X87Comparison::less && !(attributes & 16384U))
                accept = false;
        }
        if (accept) {
            const auto &gravity = o.gravity(Access::ContactGateAcceptGravity);
            if ((o.dot(gravity, o.surface_normal(Access::Known))).compare(0.0f) !=
                    X87Comparison::greater ||
                (o.registered_status(Access::ContactGateAcceptRegisteredStatus) & 3U)) {
                o.restore_radius();
                o.restore_displacement();
                return true;
            }
        }
    }
    o.restore_contact();
    return false;
}
// NOTSA: the floor-angle owner publishes inclination even if later arithmetic fails.
void store_floor_angles(Context &o, const NinjaVector &normal, Access calculation) {
    o.store_floor_angles(normal, calculation);
}

// NOTSA: absent resource IDs are actual null tasks.
TaskHandle geometry_task(Context &o, const SceneResourceReference &ref, Access calculation) {
    if (!ref.id)
        return {};
    return o.geometry_task(ref, calculation);
}

// NOTSA: named phase shared by the recovered scene dispatchers.
void project_foot_position(Context &o, const SceneResourceReference &moving, bool alternate) {
    const auto horizontal =
        o.slide_vector(alternate ? Access::AlternateFootProjectionPlane
                                 : Access::OrdinaryFootProjectionSlide);
    const auto plane =
        o.corrected_plane(alternate ? Access::AlternateFootProjectionPlane
                                    : Access::OrdinaryFootProjectionPlane);
    auto &center = o.center(alternate ? Access::AlternateFootProjectionCenter
                                      : Access::OrdinaryFootProjectionCenter);
    center.x = (alternate ? (o.exact(plane.x) + horizontal.x) : (o.exact(horizontal.x) + plane.x))
                   .rounded();
    center.y = ((o.exact(horizontal.y) + plane.y)).rounded();
    center.z = ((o.exact(horizontal.z) + plane.z)).rounded();
    const auto selected =
        o.selected_contact(alternate ? Access::FootProjectionSelectedContact
                                     : Access::OrdinaryFootProjectionPlane);
    o.require_selected_response(selected, alternate ? Access::AlternateFootProjectionSelectedResponse
                                                    : Access::OrdinaryFootProjectionSelectedResponse);
    const auto normal = o.direction(alternate ? Access::AlternateFootProjectionDirection
                                              : Access::OrdinaryFootProjectionDirection);
    NinjaVector projected;
    if (!alternate)
        o.displacement(NinjaVector{0, 0, 0});
    o.project_to_response_plane(center, selected, normal, projected);
    const auto &offset = o.result.half_height_offset;
    center.x = ((o.exact(projected.x) + offset.x)).rounded();
    center.y = ((o.exact(projected.y) + offset.y)).rounded();
    center.z = ((o.exact(projected.z) + offset.z)).rounded();
    o.moving_task(geometry_task(o, moving,
                                alternate ? Access::AlternateFootProjectionGeometryTask
                                          : Access::OrdinaryFootProjectionGeometryTask));
    if (moving.id)
        o.move_platform(offset,
                        alternate ? Access::AlternateFootProjectionMovePlatform
                                  : Access::OrdinaryFootProjectionMovePlatform);
    const NinjaVector candidate{((o.exact(center.x) - offset.x)).rounded(),
                                ((o.exact(center.y) - offset.y)).rounded(),
                                ((o.exact(center.z) - offset.z)).rounded()};
    const auto &position = o.position(alternate ? Access::AlternateFootProjectionPosition
                                                : Access::OrdinaryFootProjectionPosition);
    const auto distance = [&]() {
        if (alternate) {
            const auto dy = (o.exact(position.y) - candidate.y);
            const auto dx = (o.exact(position.x) - candidate.x);
            const auto dz = (o.exact(position.z) - candidate.z);
            const auto zz = (dz * dz), xx = (dx * dx);
            const auto sum = (zz + xx);
            const auto yy = (dy * dy);
            return o.signed_sqrt(((sum + yy)).rounded());
        } else {
            const auto dz = (o.exact(position.z) - candidate.z);
            const auto dy = (o.exact(position.y) - candidate.y);
            const auto dx = (o.exact(position.x) - candidate.x);
            const auto xx = (dx * dx), yy = (dy * dy);
            const auto sum = (xx + yy);
            const auto zz = (dz * dz);
            return o.signed_sqrt(((sum + zz)).rounded());
        }
    }();
    const auto override =
        alternate ? 0U : o.motion_override(Access::FootProjectionMotionOverride);
    const bool changed = moving.id ||
                         o.compare(o.motion.speed.x, 0.0f) != X87Comparison::equal ||
                         o.compare(o.motion.speed.z, 0.0f) != X87Comparison::equal ||
                         distance.compare(1.0f) == X87Comparison::greater || override;
    if (changed &&
        (distance.compare(o.tuning.foot_position_minimum) == X87Comparison::greater || override))
        o.position(candidate);
}
// NOTSA: named phase shared by the recovered scene dispatchers.
void initial_scene(Context &o, bool alternate) {
    o.remember_flags(
        o.status(alternate ? Access::AlternateInitialStatus : Access::OrdinaryInitialStatus));
    o.bind_contact(alternate ? Access::AlternateInitialBindContact : Access::OrdinaryInitialBindContact);
    o.status(o.orientation.flags);
    o.orientation.flags &= 65534U;
    bool floor = false;
    const NinjaVector *normal = nullptr;
    std::optional<float> initial_half;
    if (!alternate) {
        o.direction(NinjaVector{0, 1, 0});
        o.special_surface(0);
        o.contact_pitch(o.orientation.angles[0]);
        o.contact_roll(o.orientation.angles[2]);
        initial_half = ((o.exact(o.parameters.height) * 0.5f)).rounded();
        floor = (o.prior_attributes(Access::InitialPriorAttributes) & 16384U) != 0;
        if (floor)
            normal = &o.floor_normal(Access::InitialFloorNormal);
    }
    contact_offsets(o, alternate, floor, normal, initial_half);
    reset_sweep(o, alternate ? Access::AlternateInitialSweepCenter
                             : Access::OrdinaryInitialSweepCenter);
    o.radius(((o.exact(o.parameters.height) * 0.5f)).rounded());
    o.remember_position(
        o.position(alternate ? Access::AlternateInitialPosePosition : Access::OrdinaryInitialPosePosition));
    o.broadphase(alternate ? Access::AlternateInitialBroadphase
                           : Access::OrdinaryInitialBroadphase);
    first_scene_pass(o, alternate);
}
// NOTSA: named phase shared by the recovered scene dispatchers.
bool early_scene_exit(Context &o, bool alternate) {
    if (o.result.processed_count)
        return false;
    add_displacement(o, true);
    o.attributes(0, alternate ? Access::AlternateEmptyLandscapeAttributes
                              : Access::OrdinaryEmptyLandscapeAttributes);
    o.moving_task(TaskHandle{});
    o.cached_object(SceneResourceReference{});
    o.result.outcome = 0;
    return true;
}
// NOTSA: named phase shared by the recovered scene dispatchers.
void alternate_scene_pass(Context &o) {
    order_scene_pass(o, true);
    reset_sweep(o, Access::AlternateContactSweepCenter);
    o.moving_task(TaskHandle{});
    bool landed = false;
    SceneResourceReference cached;
    std::int32_t accumulator = 0;
    for (std::int32_t index = 0; index < static_cast<std::int32_t>(o.result.processed_count);
         ++index) {
        o.result.response_index = static_cast<std::size_t>(index);
        const auto entry = o.entry(EntryCollection::Ordered, static_cast<std::size_t>(index),
                                   Access::AlternateContactResponseEntry);
        if (!o.query(entry, Access::AlternateContactResponseQuery))
            continue;
        const auto attributes = o.entry_attributes(entry, Access::AlternateContactResponseEntryAttributes);
        const auto &normal = o.surface_normal(Access::AlternateContactResponseSlopeSurfaceNormal);
        const auto &direction = o.direction(Access::AlternateContactResponseSlopeDirection);
        const auto slope = o.angle(direction, normal, nullptr, Access::AlternateContactResponseSlopeAngle);
        bool wall = (o.orientation.flags & 2U) != 0;
        if (!wall && (attributes & 2097216U)) {
            if (!(attributes & 2097152U) ||
                o.environment(Access::ContactResponseEnvironment) != 3)
                wall = true;
            else
                wall = (o.exact(o.tuning.alternate_wall_normal_y)).compare(normal.y) !=
                       X87Comparison::less;
        }
        if (!wall && slope > 11264)
            wall = (o.exact(direction.y)).compare(normal.y) != X87Comparison::less;
        const auto flags = o.status(Access::AlternateContactResponseStatus);
        if (!wall && (o.result.previous_flags & 3U) && !(flags & 3U))
            wall = true;
        if (!wall) {
            const auto &gravity = o.gravity(Access::AlternateContactGravityGateGravity);
            wall = (o.dot(gravity, normal)).compare(0.0f) == X87Comparison::greater &&
                   !(flags & 3U);
        }
        if (!wall) {
            const auto vertical = o.compare(o.motion.speed.y, 0.0f);
            if (vertical == X87Comparison::greater || vertical == X87Comparison::unordered) {
                const auto vertical_recheck = o.compare(o.motion.speed.y, 0.0f);
                wall = vertical_recheck != X87Comparison::greater || !(flags & 3U);
            }
        }
        if (wall) {
            accumulator = corner_scene_response(
                o, o.result.temporary_vector, o.result.half_height_offset, accumulator, attributes);
            o.result.provisional_outcome = accumulator;
            continue;
        }
        landed = true;
        cached = o.entry_object(entry, Access::AlternateContactResponseEntryObject);
        const auto returned =
            o.respond(Response::AlternateLanding, attributes, Access::AlternateContactResponseResponse);
        if (returned && (accumulator == 0 || returned == 2))
            accumulator = returned;
        o.result.provisional_outcome = accumulator;
        store_floor_angles(o, normal, Access::AlternateFloorTurnStoreFloorAngles);
        bool rotate = true;
        const auto &speed = o.world_velocity(Access::AlternateFloorTurnCheckWorld);
        if (o.compare(speed.x, 0.0f) == X87Comparison::equal &&
            o.compare(speed.z, 0.0f) == X87Comparison::equal &&
            (o.status(Access::AlternateFloorTurnCheckStatus) & 1U)) {
            const auto &gravity = o.gravity(Access::AlternateFloorTurnCheckGravity);
            const float old = (o.dot(gravity, o.direction(Access::Known))).rounded();
            rotate = (o.dot(gravity, o.surface_normal(Access::Known))).compare(old) ==
                     X87Comparison::greater;
        }
        if (rotate)
            o.rotate_floor(Access::AlternateFloorTurnRotateFloor);
        if (accumulator == 0)
            contact_offsets(o, true, false, nullptr);
        o.require_world(Access::AlternateFloorMotionStoresWorld);
        o.world_pitch(o.contact_pitch(Access::AlternateFloorMotionStoresContactPitch));
        o.world_roll(o.contact_roll(Access::AlternateFloorMotionStoresContactRoll));
        o.floor_normal(o.surface_normal(Access::AlternateFloorMotionStoresSurfaceNormal));
        auto &horizontal = o.slide_vector(Access::AlternateSlideClampSlideVector);
        if ((o.length(horizontal)).compare(o.tuning.horizontal_contact_limit) ==
            X87Comparison::greater) {
            o.normalize(horizontal);
            const auto excess = (o.length(o.displacement(Access::AlternateSlideClampDisplacement)) -
                                 o.tuning.horizontal_contact_limit);
            auto &displacement = o.displacement(Access::Known);
            displacement.x = ((excess * horizontal.x)).rounded();
            displacement.y = ((excess * horizontal.y)).rounded();
            displacement.z = ((excess * horizontal.z)).rounded();
            horizontal.x = ((o.exact(horizontal.x) * o.tuning.horizontal_contact_limit)).rounded();
            horizontal.y = ((o.exact(horizontal.y) * o.tuning.horizontal_contact_limit)).rounded();
            horizontal.z = ((o.exact(horizontal.z) * o.tuning.horizontal_contact_limit)).rounded();
            o.swap_ordered(static_cast<std::size_t>(index), o.result.processed_count - 1);
            index = -1;
        } else
            o.displacement(NinjaVector{0, 0, 0});
        project_foot_position(o, o.entry_task(entry, Access::AlternateContactResponseCommitEntryTask), true);
        o.orientation.flags |= 1U;
        o.attributes(o.entry_attributes(entry, Access::ContactResponseCommitEntryAttributes),
                     Access::ContactResponseCommitAttributes);
    }
    if (landed) {
        o.cached_object(cached);
        add_displacement(o);
        o.result.outcome = accumulator;
    } else {
        airborne_scene_tail(o, true);
        o.result.outcome = accumulator;
    }
}
// NOTSA: named phase shared by the recovered scene dispatchers.
void normalize_scene_inline(Context &o, NinjaVector &vector) {
    const auto z = (o.exact(vector.z) * vector.z);
    const auto y = (o.exact(vector.y) * vector.y);
    const auto zy = (z + y);
    const auto x = (o.exact(vector.x) * vector.x);
    const auto length = ((zy + x)).square_root();
    if (length.compare(0.0f) == X87Comparison::equal) {
        vector.z = 0;
        vector.y = 0;
        vector.x = 0;
        return;
    }
    const auto inverse = (1.0f / length);
    vector.x = ((o.exact(vector.x) * inverse)).rounded();
    vector.y = ((o.exact(vector.y) * inverse)).rounded();
    vector.z = ((inverse * vector.z)).rounded();
}
// NOTSA: named phase shared by the recovered scene dispatchers.
bool skip_floor_turn(Context &o) {
    const auto &velocity = o.world_velocity(Access::FloorTurnCheckWorld);
    if (o.compare(velocity.x, 0.0f) != X87Comparison::equal)
        return false;
    if (o.compare(velocity.z, 0.0f) != X87Comparison::equal)
        return false;
    if (!(o.status(Access::FloorTurnCheckStatus) & 1U))
        return false;
    const auto &gravity = o.gravity(Access::FloorTurnCheckGravity);
    const float normal_dot =
        (o.dot(gravity, o.surface_normal(Access::FloorTurnCheckSurfaceNormal))).rounded();
    return (o.dot(gravity, o.direction(Access::FloorTurnCheckDirection))).compare(normal_dot) !=
           X87Comparison::greater;
}
// NOTSA: visible source scaling and CRT conversion after the recovered asin callee.
std::int32_t asin_scene_angle(Context &o, float component, double factor, Access calculation) {
    const auto angle = o.asin(component, calculation)
                           .multiply_double(o.tuning.angle_units)
                           .multiply_double(factor);
    return signed_bits(static_cast<std::uint32_t>(angle.truncated_integer()));
}

// NOTSA: named phase shared by the recovered scene dispatchers.
void ordinary_floor_turn(Context &o, std::uint32_t attributes) {
    const auto &normal = o.surface_normal(Access::FloorTurnSurfaceNormal);
    if (attributes & 16384U) {
        o.orientation.angles[2] = 0;
        o.orientation.angles[0] = 0;
        return;
    }
    if ((attributes & 131100U) || (o.status(Access::FloorTurnStatus) & 8192U)) {
        store_floor_angles(o, normal, Access::OrdinaryFloorTurnStoreFloorAngles);
        if (skip_floor_turn(o))
            return;
    } else {
        o.contact_pitch(asin_scene_angle(o, normal.z, o.tuning.positive_angle_factor,
                                         Access::FloorTurnPitchAsin));
        const auto sign = o.compare(normal.y, 0.0f);
        const auto z = asin_scene_angle(o, normal.x, o.tuning.negative_angle_factor,
                                        less_or_unordered(sign) ? Access::FloorTurnInvertedRollAsin
                                                    : Access::FloorTurnRollAsin);
        o.contact_roll(less_or_unordered(sign) ? signed_bits(32768U - static_cast<std::uint32_t>(z)) : z);
        if (skip_floor_turn(o))
            return;
        const auto x = asin_scene_angle(o, normal.z, o.tuning.positive_angle_factor,
                                        Access::FloorTurnReflectedPitchAsin);
        const auto second =
            asin_scene_angle(o, normal.x, o.tuning.negative_angle_factor,
                             Access::FloorTurnReflectedRollAsin);
        if (o.compare(normal.y, 0.0f) == X87Comparison::less) {
            const auto reflected_x = signed_bits(32768U - static_cast<std::uint32_t>(x));
            const auto reflected_z = signed_bits(32768U - static_cast<std::uint32_t>(second));
            if (angle_distance(o.orientation.angles[0], reflected_x) <
                angle_distance(o.orientation.angles[2], reflected_z)) {
                o.contact_pitch(reflected_x);
                o.contact_roll(second);
            } else {
                o.contact_pitch(x);
                o.contact_roll(reflected_z);
            }
        }
    }
    o.rotate_floor(Access::OrdinaryFloorTurnRotateFloor);
}
// NOTSA: named phase shared by the recovered scene dispatchers.
void ordinary_landing_flags(Context &o, std::uint32_t attributes) {
    const auto speed = o.motion.speed.x;
    bool ground = !less_or_unordered(o.compare(speed, o.tuning.landing_forward_speed)) ||
                  !greater_or_unordered(o.compare(speed, o.tuning.landing_reverse_speed));
    const auto &gravity = o.gravity(Access::LandingGravity);
    const auto &direction = o.direction(Access::LandingDirection);
    if (!ground)
        ground = (o.dot(gravity, direction)).compare(o.tuning.landing_gravity_alignment) ==
                 X87Comparison::less;
    if (!ground)
        ground = (attributes & 16384U) != 0;
    if (!ground) {
        const auto upper = o.compare(speed, o.tuning.landing_upper_speed);
        bool ordinary = less_or_unordered(upper) &&
                        greater_or_unordered(o.compare(speed, o.tuning.landing_lower_speed));
        if (!ordinary)
            ordinary = (o.dot(gravity, direction)).compare(o.tuning.landing_gravity_alignment) !=
                       X87Comparison::less;
        if (ordinary) {
            o.attributes(0, Access::LandingAirborneAttributes);
            if (o.compare(o.motion.speed.x, 0.0f) == X87Comparison::greater)
                o.nudge_normal(Access::LandingAirborneNudgeNormal);
            return;
        }
        ground = true;
    }
    if (ground) {
        o.orientation.flags |= 1U;
        o.attributes(attributes, Access::LandingGroundedAttributes);
    }
}
// NOTSA: named phase shared by the recovered scene dispatchers.
void ordinary_scene_pass(Context &o) {
    if (o.event_task(Access::ContactResponseEventTask))
        o.special_surface(0);
    order_scene_pass(o, false);
    reset_sweep(o, Access::OrdinaryContactSweepCenter);
    o.moving_task(TaskHandle{});
    bool landed = false;
    SceneResourceReference cached, last_object;
    std::int32_t accumulator = 0;
    for (std::size_t index = 0; index < o.result.processed_count; ++index) {
        o.result.response_index = index;
        const auto entry = o.entry(EntryCollection::Ordered, index, Access::OrdinaryContactResponseEntry);
        if (!o.query(entry, Access::OrdinaryContactResponseQuery))
            continue;
        const auto attributes =
            o.entry_attributes(entry, Access::OrdinaryContactResponseEntryAttributes);
        const auto object =
            o.entry_object(entry, Access::OrdinaryContactResponseEntryObject);
        const auto special =
            o.special_surface(Access::ContactResponseSpecialSurface);
        std::uint32_t decisions = 1;
        if ((o.orientation.flags & 1U) && last_object.id != object.id) {
            const auto &horizontal = o.slide_vector(Access::ContactDeltaSlideVector);
            const auto &plane =
                (special == 1 ? o.response_plane(Access::ContactDeltaResponsePlane)
                              : o.corrected_plane(Access::ContactDeltaResponsePlane));
            const auto &position = o.position(Access::ContactDeltaPosition);
            const auto &half = o.result.half_height_offset;
            const auto component = [&](float a, float b, float h, float p) {
                auto value = (o.exact(a) + b);
                if (special != 1)
                    value = (value - h);
                return ((value - p)).rounded();
            };
            NinjaVector delta{component(horizontal.x, plane.x, half.x, position.x),
                              component(horizontal.y, plane.y, half.y, position.y),
                              component(horizontal.z, plane.z, half.z, position.z)};
            const auto length = o.length(delta);
            const auto spill = length.rounded();
            if (length.compare(0.0f) != X87Comparison::equal) {
                normalize_scene_inline(o, delta);
                if ((o.dot(delta, o.surface_normal(Access::ContactDeltaSurfaceNormal)))
                            .compare(o.tuning.contact_delta_alignment) == X87Comparison::less ||
                    o.compare(spill, o.parameters.height) == X87Comparison::greater)
                    decisions = 0;
            }
        }
        decisions |= 4U;
        if ((o.orientation.flags & 3U) && !(o.orientation.flags & 8192U)) {
            const auto &horizontal = o.slide_vector(Access::ContactForwardDeltaSlideVector);
            const auto &plane = o.response_plane(Access::ContactForwardDeltaResponsePlane);
            const auto &center = o.center(Access::ContactForwardDeltaCenter);
            NinjaVector delta{(((o.exact(horizontal.x) + plane.x) - center.x)).rounded(),
                              special ? 0
                                      : (((o.exact(horizontal.y) + plane.y) - center.y)).rounded(),
                              (((o.exact(horizontal.z) + plane.z) - center.z)).rounded()};
            if ((o.length(delta)).compare(0.0f) != X87Comparison::equal) {
                normalize_scene_inline(o, delta);
                NinjaVector forward{
                    less_or_unordered(o.compare(o.motion.speed.x, 0.0f)) ? -1.0F : 1.0F, 0, 0};
                o.task_to_world(forward, Access::ContactForwardDeltaTaskToWorld);
                const auto loaded_y = o.exact(forward.y);
                const auto absolute = loaded_y.is_negative() ? (-loaded_y) : loaded_y;
                if (!special || absolute.compare(1.0f) != X87Comparison::equal) {
                    normalize_scene_inline(o, forward);
                    if ((o.dot(delta, forward)).compare(o.tuning.forward_contact_alignment) ==
                        X87Comparison::less)
                        decisions &= ~4U;
                }
            }
        }
        auto slope = o.angle(o.direction(Access::OrdinaryContactResponseSlopeDirection),
                             o.surface_normal(Access::OrdinaryContactResponseSlopeSurfaceNormal), nullptr,
                             Access::OrdinaryContactResponseSlopeAngle);
        const bool steep = slope > 11264;
        if (steep) {
            (void)scene_contact_gate(o, entry);
            slope = o.angle(o.direction(Access::Known), o.surface_normal(Access::Known),
                            nullptr, Access::ContactResponseRecheckAngle);
            decisions |= 2U;
        }
        bool wall = (o.orientation.flags & 2U) || (attributes & 2097216U);
        if (!wall && (slope > 11264 || (decisions & 5U) != 5U)) {
            const auto &gravity = o.gravity(Access::ContactSlopeGravity);
            const auto old = (o.dot(gravity, o.direction(Access::Known))).rounded();
            if ((o.dot(gravity, o.surface_normal(Access::Known))).compare(old) !=
                X87Comparison::less)
                wall = !(attributes & 16384U) || (decisions & 5U) != 5U;
        }
        const auto flags = o.status(Access::OrdinaryContactResponseStatus);
        if (!wall && (o.result.previous_flags & 3U) && !(flags & 3U))
            wall = true;
        if (!wall) {
            const auto &gravity = o.gravity(Access::ContactCeilingGravity);
            wall = (o.dot(gravity, o.surface_normal(Access::Known))).compare(0.0f) ==
                       X87Comparison::greater &&
                   !(flags & 3U);
        }
        if (!wall && !steep && !scene_contact_gate(o, entry))
            wall = true;
        if (!wall && o.character_id == 7 && o.action(Access::ContactResponseAction) == 48)
            wall = true;
        if (!wall &&
            !less_or_unordered((o.length(o.world_velocity(Access::ContactResponseWorld)))
                       .compare(o.parameters.run_speed)) &&
            !(attributes & 16384U)) {
            auto velocity = o.world_velocity(Access::Known);
            o.normalize(velocity);
            wall = (o.dot(velocity, o.surface_normal(Access::Known)))
                       .compare(o.tuning.fast_wall_alignment) == X87Comparison::greater;
        }
        if (!wall) {
            const auto vertical = o.compare(o.motion.speed.y, 0.0f);
            if (vertical == X87Comparison::greater || vertical == X87Comparison::unordered) {
                const auto vertical_recheck = o.compare(o.motion.speed.y, 0.0f);
                wall = vertical_recheck != X87Comparison::greater || !(flags & 3U);
            }
        }
        if (wall) {
            accumulator = wall_scene_response(o, o.result.temporary_vector,
                                              o.result.half_height_offset, accumulator, attributes);
            o.result.provisional_outcome = accumulator;
            continue;
        }
        landed = true;
        cached = object;
        last_object = object;
        const auto returned =
            o.respond(Response::OrdinaryLanding, attributes, Access::OrdinaryContactResponseResponse);
        if (returned && (accumulator == 0 || returned == 2))
            accumulator = returned;
        o.result.provisional_outcome = accumulator;
        ordinary_floor_turn(o, attributes);
        if (accumulator == 0) {
            const bool floor = (attributes & 16384U) != 0;
            const auto *normal =
                floor ? &o.surface_normal(Access::ContactOffsetFloorSurfaceNormal) : nullptr;
            contact_offsets(o, false, floor, normal);
        }
        o.require_world(Access::OrdinaryFloorMotionStoresWorld);
        o.world_pitch(o.contact_pitch(Access::OrdinaryFloorMotionStoresContactPitch));
        o.world_roll(o.contact_roll(Access::OrdinaryFloorMotionStoresContactRoll));
        o.floor_normal(o.surface_normal(Access::OrdinaryFloorMotionStoresSurfaceNormal));
        if (attributes & 16384U) {
            auto horizontal = o.slide_vector(Access::FloorSlideClampSlideVector);
            if ((o.length(horizontal)).compare(0.0f) != X87Comparison::equal) {
                const float length =
                    (o.length(o.displacement(Access::FloorSlideClampDisplacement))).rounded();
                o.normalize(horizontal);
                auto &target = o.slide_vector(Access::Known);
                target.x = ((o.exact(length) * horizontal.x)).rounded();
                target.y = ((o.exact(length) * horizontal.y)).rounded();
                target.z = ((o.exact(length) * horizontal.z)).rounded();
            }
        }
        project_foot_position(o, o.entry_task(entry, Access::OrdinaryContactResponseCommitEntryTask), false);
        ordinary_landing_flags(o, attributes);
    }
    if (landed) {
        o.cached_object(cached);
        add_displacement(o);
        o.result.outcome = accumulator;
    } else {
        airborne_scene_tail(o, false);
        o.result.outcome = accumulator;
    }
}

} // namespace
// FUNCTION: SONIC 0x449500 SomethingAboutWater
std::int32_t position_player_in_scene(Context &context) {
    initial_scene(context, false);
    if (!early_scene_exit(context, false))
        ordinary_scene_pass(context);
    return context.result.outcome;
}
// FUNCTION: SONIC 0x445d10 SomethingAboutWaterButNotReally
std::int32_t position_player_in_scene_alternate(Context &context) {
    initial_scene(context, true);
    if (!early_scene_exit(context, true))
        alternate_scene_pass(context);
    return context.result.outcome;
}
} // namespace sadx::scene
