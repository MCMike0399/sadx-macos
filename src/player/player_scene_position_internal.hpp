#pragma once
#include "cpu/fidelity_float.hpp"
#include "player/player_plane.hpp"
#include "player/player_scene_position.hpp"
#include <cstring>

namespace sadx::scene {
enum class Access {
    Known,
    WallSlideSpeedGateStatus,
    WallSlideCrossSurfaceNormal,
    WallSlideFallbackDirection,
    WallSlidePlanePoint,
    WallSlidePlaneNormal,
    WallSlidePlaneCenter,
    WallSlideVelocitySlideVector,
    WallSlideGroundDisplacement,
    WallSlideRegisteredVelocityPriorAttributes,
    WallSlideRegisteredVelocityRegisteredVelocity,
    WallSlideTurnDisplacement,
    WallSlideTurnDirection,
    WallSlideTurnAngle,
    WallSlideTurnRegisteredYaw,
    CornerSlideStatus,
    CornerSlideSurfaceNormal,
    CornerSlideDisplacement,
    CornerSlideSlideVector,
    CornerSlideTurnDirection,
    CornerSlideTurnAngle,
    CornerSlideTargetYaw,
    CornerSlideRegisteredVelocity,
    WallWorldDirectionWorld,
    WallPositionDirectionPosition,
    WallPositionCommitCorrectedPlane,
    WallPositionCommitPosition,
    WallPositionCommitSlideVector,
    WallResponseSurfaceNormal,
    GroundedWallResponseAngle,
    WallResponseMotionOverride,
    WallResponseTaskToWorld,
    AirborneWallResponseAngle,
    CornerResponseSurfaceNormal,
    GroundedCornerResponseAngle,
    AirborneCornerResponseAngle,
    ContactOffsetDirectionCalculateVector,
    ContactOffsetHalfHeightCalculateVector,
    AlternateContactOffsetActorInitializeOffset,
    ContactOffsetCenterCalculateVector,
    ContactOffsetDirectionFloorToWorld,
    ContactOffsetHalfHeightFloorToWorld,
    ContactOffsetDirectionTaskToWorld,
    ContactOffsetHalfHeightTaskToWorld,
    ContactOffsetFloorInitializeOffset,
    OrdinaryContactOffsetActorInitializeOffset,
    ContactOffsetCenterFloorToWorld,
    ContactOffsetCenterTaskToWorld,
    MovePositionPosition,
    MovePositionWorld,
    MovePositionDisplacement,
    AlternateCandidateAdmissionEntryAttributes,
    OrdinaryCandidateAdmissionEntryAttributes,
    AlternateCandidateAdmissionEntryTask,
    OrdinaryCandidateAdmissionEntryTask,
    AlternateCandidateAdmissionGeometryTask,
    OrdinaryCandidateAdmissionGeometryTask,
    CandidateAdmissionNullGeometryPrimary,
    CandidateAdmissionGeometryFlags,
    AlternateFirstPassQueryQuery,
    OrdinaryFirstPassQueryQuery,
    AlternateFirstPassQueryStatus,
    OrdinaryFirstPassQueryStatus,
    AlternateFirstPassGroundedResponseResponse,
    OrdinaryFirstPassGroundedResponseResponse,
    AlternateFirstPassAirborneResponseResponse,
    OrdinaryFirstPassAirborneResponseResponse,
    AlternateFirstPassCapacityLocalBufferOverflow,
    OrdinaryFirstPassCapacityLocalBufferOverflow,
    AlternateCandidateOrderMovingTask,
    OrdinaryCandidateOrderMovingTask,
    AlternateCandidateOrderEntry,
    OrdinaryCandidateOrderEntry,
    AlternateCandidateOrderCachedObject,
    OrdinaryCandidateOrderCachedObject,
    AlternateCandidateOrderLocalBufferOverflow,
    OrdinaryCandidateOrderLocalBufferOverflow,
    AlternateAirborneTailStatus,
    OrdinaryAirborneTailStatus,
    AlternateAirborneTailInput,
    OrdinaryAirborneTailInput,
    AlternateAirborneTailNoControlTime,
    OrdinaryAirborneTailNoControlTime,
    AirborneTailPriorAttributes,
    AlternateAirborneTailPosition,
    OrdinaryAirborneTailPosition,
    AlternateAirborneTailWorld,
    OrdinaryAirborneTailWorld,
    AlternateAirborneTailAttributes,
    OrdinaryAirborneTailAttributes,
    ContactGateRegisteredMotion,
    ContactGateRegisteredPlayer,
    ContactGateCenterSpecialSurface,
    ContactGateCenterAmyCenterOffset,
    ContactGateRegisteredParameters,
    ContactGateCenterInitializeOffset,
    ContactGateCenterSurfaceNormal,
    ContactGateCenterFloorToWorld,
    ContactGateCenterTaskToWorld,
    ContactGateRadiusRadius,
    ContactGateRadiusAmyRadiusOffset,
    ContactGateRadiusFloorToWorld,
    ContactGateRadiusTaskToWorld,
    ContactGateSweepEntryAttributes,
    ContactGateSweepCenter,
    ContactGateSweepCorrectedPlane,
    ContactGateSweepSlideVector,
    ContactGateSlopeDirection,
    ContactGateSlopeSurfaceNormal,
    ContactGateSlopeAngle,
    ContactGateSlopeSlideVector,
    ContactGateSlopeDisplacement,
    ContactGateUnalignedCenterResponsePlane,
    ContactGateUnalignedCenterCorrectedPlane,
    ContactGateUnalignedCenterPosition,
    ContactGateRadiusRegisteredAttributes,
    ContactGateRadiusRenderMode,
    ContactGateCallAgainAgainAgain,
    ContactGateQueryQuery,
    ContactGateQueryDirection,
    ContactGateQuerySurfaceNormal,
    ContactGateQueryAngle,
    ContactGateQueryGravity,
    ContactGateAcceptGravity,
    ContactGateAcceptRegisteredStatus,
    AlternateFootProjectionPlane,
    OrdinaryFootProjectionSlide,
    OrdinaryFootProjectionPlane,
    AlternateFootProjectionCenter,
    OrdinaryFootProjectionCenter,
    FootProjectionSelectedContact,
    AlternateFootProjectionSelectedResponse,
    OrdinaryFootProjectionSelectedResponse,
    AlternateFootProjectionDirection,
    OrdinaryFootProjectionDirection,
    FootProjectionDistance,
    FootProjectionPlaneProjection,
    AlternateFootProjectionGeometryTask,
    OrdinaryFootProjectionGeometryTask,
    AlternateFootProjectionMovePlatform,
    OrdinaryFootProjectionMovePlatform,
    AlternateFootProjectionPosition,
    OrdinaryFootProjectionPosition,
    FootProjectionMotionOverride,
    AlternateInitialStatus,
    OrdinaryInitialStatus,
    AlternateInitialBindContact,
    OrdinaryInitialBindContact,
    InitialPriorAttributes,
    InitialFloorNormal,
    AlternateInitialSweepCenter,
    OrdinaryInitialSweepCenter,
    AlternateInitialPosePosition,
    OrdinaryInitialPosePosition,
    AlternateInitialBroadphase,
    OrdinaryInitialBroadphase,
    AlternateEmptyLandscapeAttributes,
    OrdinaryEmptyLandscapeAttributes,
    AlternateContactSweepCenter,
    AlternateContactResponseEntry,
    AlternateContactResponseQuery,
    AlternateContactResponseEntryAttributes,
    AlternateContactResponseSlopeSurfaceNormal,
    AlternateContactResponseSlopeDirection,
    AlternateContactResponseSlopeAngle,
    ContactResponseEnvironment,
    AlternateContactResponseStatus,
    AlternateContactGravityGateGravity,
    AlternateContactResponseEntryObject,
    AlternateContactResponseResponse,
    AlternateFloorTurnStoreFloorAngles,
    AlternateFloorTurnCheckWorld,
    AlternateFloorTurnCheckStatus,
    AlternateFloorTurnCheckGravity,
    AlternateFloorTurnRotateFloor,
    AlternateFloorMotionStoresWorld,
    AlternateFloorMotionStoresContactPitch,
    AlternateFloorMotionStoresContactRoll,
    AlternateFloorMotionStoresSurfaceNormal,
    AlternateSlideClampSlideVector,
    AlternateSlideClampDisplacement,
    AlternateContactResponseCommitEntryTask,
    ContactResponseCommitEntryAttributes,
    ContactResponseCommitAttributes,
    FloorTurnCheckWorld,
    FloorTurnCheckStatus,
    FloorTurnCheckGravity,
    FloorTurnCheckSurfaceNormal,
    FloorTurnCheckDirection,
    FloorTurnSurfaceNormal,
    FloorTurnStatus,
    OrdinaryFloorTurnStoreFloorAngles,
    FloorTurnPitchAsin,
    FloorTurnInvertedRollAsin,
    FloorTurnRollAsin,
    FloorTurnReflectedPitchAsin,
    FloorTurnReflectedRollAsin,
    OrdinaryFloorTurnRotateFloor,
    LandingGravity,
    LandingDirection,
    LandingAirborneAttributes,
    LandingAirborneNudgeNormal,
    LandingGroundedAttributes,
    ContactResponseEventTask,
    OrdinaryContactSweepCenter,
    OrdinaryContactResponseEntry,
    OrdinaryContactResponseQuery,
    OrdinaryContactResponseEntryAttributes,
    OrdinaryContactResponseEntryObject,
    ContactResponseSpecialSurface,
    ContactDeltaSlideVector,
    ContactDeltaResponsePlane,
    ContactDeltaPosition,
    ContactDeltaSurfaceNormal,
    ContactForwardDeltaSlideVector,
    ContactForwardDeltaResponsePlane,
    ContactForwardDeltaCenter,
    ContactForwardDeltaTaskToWorld,
    OrdinaryContactResponseSlopeDirection,
    OrdinaryContactResponseSlopeSurfaceNormal,
    OrdinaryContactResponseSlopeAngle,
    ContactResponseRecheckAngle,
    ContactSlopeGravity,
    OrdinaryContactResponseStatus,
    ContactCeilingGravity,
    ContactResponseAction,
    ContactResponseWorld,
    OrdinaryContactResponseResponse,
    ContactOffsetFloorSurfaceNormal,
    OrdinaryFloorMotionStoresWorld,
    OrdinaryFloorMotionStoresContactPitch,
    OrdinaryFloorMotionStoresContactRoll,
    OrdinaryFloorMotionStoresSurfaceNormal,
    FloorSlideClampSlideVector,
    FloorSlideClampDisplacement,
    OrdinaryContactResponseCommitEntryTask
};

// NOTSA: source compiler-pool constants with literal native defaults.
struct Constants {
    float wall_alignment = 4.0f;
    float push_distance = 3.0f;
    float big_radius_addition = 0.3f;
    float corner_speed_minimum = 0.4f;
    float contact_delta_alignment = -0.2f;
    float backstep_alignment = 0.7f;
    float corner_fast_speed = 0.6f;
    float turn_direction_minimum = 0.2f;
    float active_input_minimum = 0.9f;
    float wall_axis_minimum = 0.15f;
    float corner_speed_multiplier = 0.96f;
    float foot_position_minimum = 0.0001f;
    float forward_contact_alignment = -0.4f;
    float alternate_wall_normal_y = 0.99f;
    float horizontal_contact_limit = 4.1f;
    float wall_stop_speed = 15.0f;
    float landing_lower_speed = -1.16f;
    float landing_upper_speed = 1.16f;
    float landing_gravity_alignment = -0.34f;
    float landing_reverse_speed = -1.85f;
    float landing_forward_speed = 1.85f;
    float fast_wall_alignment = 0.52f;
    double positive_angle_factor = 0.15915497620314795;
    double angle_units = 65536.0;
    double negative_angle_factor = -0.15915497620314795;
};
enum class EntryCollection { Candidates, FirstPass, Ordered };
struct EntrySlot {
    EntryCollection collection;
    std::size_t index;
};
enum class ActorSelection { Passed, Registered };
enum class Response {
    OrdinaryGrounded,
    AlternateGrounded,
    Airborne,
    OrdinaryLanding,
    AlternateLanding
};
struct Input {
    bool active;
    float magnitude;
};
// NOTSA: computed call-local values; optional winner means no winning entry in the game.
struct Scratch {
    std::int32_t outcome = 0, provisional_outcome = 0;
    std::uint16_t previous_flags = 0;
    NinjaVector center_offset{}, half_height_offset{}, temporary_vector{};
    std::size_t candidate_index = 0, processed_count = 0, response_index = 0;
    std::optional<std::size_t> winning_index;
};
// NOTSA: typed lazy reads and immediate stores. Reference backends keep unknown facts here.
struct Context {
    Context(PlayerOrientation &orientation, PlayerMotion &motion, PlayerParameters &parameters,
            std::uint8_t &index, std::uint8_t &character, float &alignment,
            const Constants &constants, PlayerCoordinates &coordinates, X87Context &fp,
            CoordinateAxisConstants axis)
        : orientation(orientation), motion(motion), parameters(parameters), player_index(index),
          character_id(character), alignment(alignment), tuning(constants),
          coordinates(coordinates), fp(fp), axis(axis) {}
    virtual ~Context() = default;
    PlayerOrientation &orientation;
    PlayerMotion &motion;
    PlayerParameters &parameters;
    std::uint8_t &player_index, &character_id;
    float &alignment;
    const Constants &tuning;
    PlayerCoordinates &coordinates;
    X87Context &fp;
    CoordinateAxisConstants axis;
    Scratch result;
    fidelity::ExactFloat exact(float value) { return {fp, value}; }
    X87Comparison compare(float a, float b) { return exact(a).compare(b); }
    fidelity::ExactFloat length(const NinjaVector &value);
    fidelity::ExactFloat dot(const NinjaVector &, const NinjaVector &);
    fidelity::ExactFloat cross(const NinjaVector &, const NinjaVector &, NinjaVector &);
    fidelity::ExactFloat signed_sqrt(float value);
    void normalize(NinjaVector &value) { (void)normalize_vector_extended(value, axis, fp); }
    void subtract_angle(std::int32_t);
    void align_motion_yaw(Access);
    void scale_retained(NinjaVector &, const NinjaVector &, fidelity::ExactFloat);
    virtual void remember_flags(std::uint16_t value) { result.previous_flags = value; }
    virtual void remember_position(const NinjaVector &value) { result.temporary_vector = value; }
    virtual void remember_offsets(const NinjaVector &half, const NinjaVector &center) {
        result.half_height_offset = half;
        result.center_offset = center;
    }
    virtual NinjaVector &position(Access = Access::Known) = 0;
    virtual void position(const NinjaVector &) = 0;
    virtual std::uint8_t &action(Access = Access::Known) = 0;
    virtual std::uint16_t &status(Access = Access::Known) = 0;
    virtual void status(const std::uint16_t &) = 0;
    virtual std::uint32_t &prior_attributes(Access = Access::Known) = 0;
    virtual NinjaVector &floor_normal(Access = Access::Known) = 0;
    virtual void floor_normal(const NinjaVector &) = 0;
    virtual TaskHandle &moving_task(Access = Access::Known) = 0;
    virtual void moving_task(const TaskHandle &) = 0;
    virtual SceneResourceReference &cached_object(Access = Access::Known) = 0;
    virtual void cached_object(const SceneResourceReference &) = 0;
    virtual std::int16_t &no_control_time(Access = Access::Known) = 0;
    virtual NinjaVector &collision_normal(Access = Access::Known) = 0;
    virtual void collision_normal(const NinjaVector &) = 0;
    virtual std::uint32_t &special_surface(Access = Access::Known) = 0;
    virtual void special_surface(const std::uint32_t &) = 0;
    virtual std::uint32_t &motion_override(Access = Access::Known) = 0;
    virtual TaskHandle &event_task(Access = Access::Known) = 0;
    virtual std::uint8_t &environment(Access = Access::Known) = 0;
    virtual std::int8_t &render_mode(Access = Access::Known) = 0;
    virtual float &amy_center_offset(Access = Access::Known) = 0;
    virtual float &amy_radius_offset(Access = Access::Known) = 0;
    virtual const NinjaVector &gravity(Access = Access::Known) = 0;
    virtual float &radius(Access = Access::Known) = 0;
    virtual void radius(const float &) = 0;
    virtual NinjaVector &center(Access = Access::Known) = 0;
    virtual void center(const NinjaVector &) = 0;
    virtual NinjaVector &displacement(Access = Access::Known) = 0;
    virtual void displacement(const NinjaVector &) = 0;
    virtual NinjaVector &direction(Access = Access::Known) = 0;
    virtual void direction(const NinjaVector &) = 0;
    virtual std::int32_t &contact_pitch(Access = Access::Known) = 0;
    virtual void contact_pitch(const std::int32_t &) = 0;
    virtual std::int32_t &contact_roll(Access = Access::Known) = 0;
    virtual void contact_roll(const std::int32_t &) = 0;
    virtual NinjaVector &response_normal(Access = Access::Known) = 0;
    virtual void response_normal(const NinjaVector &) = 0;
    virtual NinjaVector &surface_normal(Access = Access::Known) = 0;
    virtual void surface_normal(const NinjaVector &) = 0;
    virtual NinjaVector &response_plane(Access = Access::Known) = 0;
    virtual void response_plane(const NinjaVector &) = 0;
    virtual NinjaVector &corrected_plane(Access = Access::Known) = 0;
    virtual void corrected_plane(const NinjaVector &) = 0;
    virtual NinjaVector &slide_vector(Access = Access::Known) = 0;
    virtual void slide_vector(const NinjaVector &) = 0;
    virtual std::uint16_t &selected_contact(Access = Access::Known) = 0;
    virtual std::uint32_t &registered_attributes(Access = Access::Known) = 0;
    virtual std::uint16_t &registered_status(Access = Access::Known) = 0;

    virtual void bind_contact(Access) = 0;
    virtual void require_world(Access) = 0;
    virtual NinjaVector &world_velocity(Access) = 0;
    virtual void world_pitch(std::int32_t) = 0;
    virtual void world_roll(std::int32_t) = 0;
    virtual NinjaVector &registered_velocity(Access) = 0;
    virtual std::int32_t &registered_yaw(Access) = 0;
    virtual void select_registered_player() = 0;
    virtual const PlayerParameters &registered_parameters() = 0;
    virtual NinjaVector &registered_player_speed() = 0;
    virtual void save_contact() = 0;
    virtual void restore_contact() = 0;
    virtual void restore_radius() = 0;
    virtual void restore_displacement() = 0;
    virtual std::int32_t candidate_count() = 0;
    virtual EntrySlot entry(EntryCollection, std::size_t, Access) = 0;
    virtual std::uint32_t entry_attributes(EntrySlot, Access) = 0;
    virtual SceneResourceReference entry_object(EntrySlot, Access) = 0;
    virtual SceneResourceReference entry_task(EntrySlot, Access) = 0;
    virtual void copy_entry(EntrySlot, EntrySlot) = 0;
    virtual void swap_ordered(std::size_t, std::size_t) = 0;
    virtual TaskHandle geometry_task(const SceneResourceReference &, Access) = 0;
    virtual bool geometry_has_primary(const TaskHandle &) = 0;
    virtual std::uint16_t geometry_flags(const TaskHandle &, Access) = 0;
    virtual void broadphase(Access) = 0;
    virtual bool query(EntrySlot, Access) = 0;
    virtual std::int32_t respond(Response, std::uint32_t, Access) = 0;
    virtual void attributes(std::uint32_t, Access) = 0;
    virtual std::int32_t angle(const NinjaVector &, const NinjaVector &, const NinjaVector *,
                               Access) = 0;
    virtual void initialize_offset(NinjaVector &, ActorSelection, Access) = 0;
    virtual void task_to_world(NinjaVector &, Access) = 0;
    virtual void calculate_vector(const NinjaMatrix &, const NinjaVector &, NinjaVector &,
                                  Access) = 0;
    virtual void floor_to_world(const NinjaVector &, NinjaVector &, Access) = 0;
    virtual void intersect_planes(const NinjaPlane &, const NinjaPlane &, NinjaPlane &) = 0;
    virtual void require_selected_response(std::uint16_t, Access) = 0;
    virtual void project_to_response_plane(const NinjaVector &, std::uint16_t, const NinjaVector &,
                                           NinjaVector &) = 0;
    virtual void store_floor_angles(const NinjaVector &, Access) = 0;
    virtual fidelity::ExactFloat asin(float, Access) = 0;
    virtual void rotate_floor(Access) = 0;
    virtual void nudge_normal(Access) = 0;
    virtual void move_platform(const NinjaVector &, Access) = 0;
    virtual Input read_input(Access) = 0;
    virtual void draw_contact_marker() = 0;
    [[noreturn]] virtual void fail(Failure, Access) = 0;
};
std::int32_t position_player_in_scene(Context &);
std::int32_t position_player_in_scene_alternate(Context &);
} // namespace sadx::scene
