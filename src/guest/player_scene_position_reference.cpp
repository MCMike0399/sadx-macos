#include "guest/player_scene_position_reference.hpp"
#include "player/player_scene_position_helpers_internal.hpp"
#include "player/player_scene_position_internal.hpp"
#include <cstring>
#include <stdexcept>

namespace sadx {
float PlayerScenePositionConstants::single(std::uint32_t address) const {
    for (std::size_t i = 0; i < float_addresses.size(); ++i)
        if (float_addresses[i] == address) {
            float value;
            std::memcpy(&value, &float_bits[i], 4);
            return value;
        }
    throw std::out_of_range("Unknown scene-position binary32 source constant");
}
double PlayerScenePositionConstants::wide(std::uint32_t address) const {
    for (std::size_t i = 0; i < double_addresses.size(); ++i)
        if (double_addresses[i] == address) {
            double value;
            std::memcpy(&value, &double_bits[i], 8);
            return value;
        }
    throw std::out_of_range("Unknown scene-position binary64 source constant");
}
std::optional<PlayerScenePositionConstants>
decode_scene_position_constants(const std::uint8_t *bytes, std::size_t size) {
    if (!bytes || size != 124)
        return {};
    PlayerScenePositionConstants value;
    std::memcpy(value.float_bits.data(), bytes, 100);
    std::memcpy(value.double_bits.data(), bytes + 100, 24);
    return value;
}
namespace {
struct Missing {
    ScenePositionFailure why;
    std::uint32_t site;
};
struct Stopped {};
template <class T> const T &need(const std::optional<T> &value, std::uint32_t site) {
    if (!value)
        throw Missing{ScenePositionFailure::unavailable_field, site};
    return *value;
}
template <class T> T &need(std::optional<T> &value, std::uint32_t site) {
    if (!value)
        throw Missing{ScenePositionFailure::unavailable_field, site};
    return *value;
}
scene::MotionConstants motion_constants(const PlayerScenePositionConstants &constants) {
    return {constants.single(0x7dccd8), constants.single(0x7dcb2c), constants.wide(0x7dccd0),
            constants.wide(0x7dccc8)};
}
template <class Function> void invoke(PlayerScenePositionResult &result, Function function) {
    try {
        function();
        result.completed = true;
    } catch (Missing missing) {
        result.failure = missing.why;
        result.source_boundary = missing.site;
    } catch (Stopped) {
    }
}

struct ReferenceNudge final : scene::NudgeContext {
    ReferenceNudge(PlayerTaskState &actor, PlayerMotionState *motion,
                   const PlayerContactWork &contact, const scene::MotionConstants &constants,
                   X87Context &fp)
        : NudgeContext(fp, constants), actor_(actor), motion_(motion), contact_(contact) {}
    PlayerTaskState &actor_;
    PlayerMotionState *motion_;
    const PlayerContactWork &contact_;
    const NinjaVector &normal() override { return need(contact_.vector_at_3c, 0x43cb40); }
    NinjaVector &position() override { return need(actor_.position, 0x43cb49); }
    MotionWork &world_motion() override {
        if (!motion_)
            throw Missing{ScenePositionFailure::null_motion, 0x43cb72};
        return motion_->motion;
    }
};

struct ReferencePlatform final : scene::PlatformContext {
    ReferencePlatform(PlayerTaskState &actor, PlayerPhysicsState &player,
                      PlayerContactWork &contact, const ScenePlatformRuntime &runtime,
                      const scene::MotionConstants &constants, X87Context &fp,
                      PlayerScenePositionResult &result)
        : PlatformContext(fp, constants), actor_(actor), player_(player), contact_(contact),
          runtime_(runtime), result_(result) {}
    PlayerTaskState &actor_;
    PlayerPhysicsState &player_;
    PlayerContactWork &contact_;
    const ScenePlatformRuntime &runtime_;
    PlayerScenePositionResult &result_;
    ScenePlatformWork *work_ = nullptr;
    ScenePlatformRow *row_ = nullptr;
    ScenePlatformCallback callback_ = nullptr;

    bool has_force_work() override {
        const auto moving = need(player_.moving_task_at_6c, 0x43ca4b);
        if (!moving)
            throw Missing{ScenePositionFailure::null_moving_task, 0x43ca4f};
        if (!moving->force_work)
            return false;
        if (!runtime_.platform_work)
            throw Missing{ScenePositionFailure::unavailable_platform_work, 0x43ca4f};
        const auto found = runtime_.platform_work->find(moving.get());
        if (found == runtime_.platform_work->end() || !found->second)
            throw Missing{ScenePositionFailure::unavailable_platform_work, 0x43ca4f};
        work_ = found->second;
        return true;
    }
    std::uint8_t player_index() override { return actor_.player_index; }
    void select_row(std::size_t index) override { row_ = &need(work_->rows[index], 0x43ca93); }
    const NinjaVector &speed() override { return player_.motion.speed; }
    const NinjaVector &position(scene::PositionUse use) override {
        return need(actor_.position, use == scene::PositionUse::IdlePlatform ? 0x43ca79 : 0x43ca9d);
    }
    void position(const NinjaVector &value) override { actor_.position = value; }
    NinjaVector &center(scene::CenterUse use) override {
        switch (use) {
        case scene::CenterUse::IdlePlatform:
            return need(contact_.center_at_4, 0x43ca7e);
        case scene::CenterUse::Callback:
            return need(contact_.center_at_4, 0x43cab4);
        case scene::CenterUse::Displacement:
            return need(contact_.center_at_4, 0x43caf0);
        }
        throw std::logic_error("unknown platform center use");
    }
    bool has_callback() override {
        callback_ = need(row_->callback, 0x43ca93);
        return callback_ != nullptr;
    }
    void save_position(const NinjaVector &saved) override { result_.temporary_vector = saved; }
    void run_callback() override {
        if (!runtime_.players || actor_.player_index >= runtime_.players->tasks.size())
            throw Missing{ScenePositionFailure::unavailable_registry_owner, 0x43caca};
        const auto player_task = need(runtime_.players->tasks[actor_.player_index], 0x43caca);
        const auto result = callback_(player_task, actor_, *row_, arithmetic);
        if (!result) {
            result_.platform_callback = std::make_shared<PlayerScenePositionResult>(result);
            result_.failure = ScenePositionFailure::platform_callback_failed;
            result_.source_boundary = 0x43cad4;
            result_.nested_source_boundary = result.source_boundary;
            throw Stopped{};
        }
    }
    float displacement(std::size_t index) override {
        constexpr std::array<std::uint32_t, 3> sites{{0x43caed, 0x43caf6, 0x43caff}};
        const auto &value = need(row_->displacement, sites[index]);
        switch (index) {
        case 0:
            return value.x;
        case 1:
            return value.y;
        default:
            return value.z;
        }
    }
    std::int32_t angle_delta(std::size_t index) override {
        return need(row_->angle_delta, 0x43cb08)[index];
    }
    PlayerOrientation &orientation() override { return actor_.orientation; }
};

struct ReferenceFloor final : scene::FloorContext {
    ReferenceFloor(PlayerTaskState &actor, PlayerPhysicsState &player,
                   const PlayerParameters &parameters, const PlayerContactWork &contact,
                   const std::optional<NinjaVector> &gravity, PlayerCoordinates &coordinates,
                   const scene::MotionConstants &constants, X87Context &fp,
                   PlayerScenePositionResult &result)
        : FloorContext(fp, constants), actor_(actor), player_(player), parameters_(parameters),
          contact_(contact), gravity_(gravity), coordinates_(coordinates), result_(result) {}
    PlayerTaskState &actor_;
    PlayerPhysicsState &player_;
    const PlayerParameters &parameters_;
    const PlayerContactWork &contact_;
    const std::optional<NinjaVector> &gravity_;
    PlayerCoordinates &coordinates_;
    PlayerScenePositionResult &result_;
    const NinjaVector &gravity() override { return need(gravity_, 0x444f24); }
    const NinjaVector &direction() override { return need(contact_.direction_at_1c, 0x444f24); }
    const NinjaVector &normal() override { return need(contact_.vector_at_48, 0x444f34); }
    std::uint16_t flags() override { return need(player_.flags_at_10, 0x444f4a); }
    const NinjaVector &speed() override { return player_.motion.speed; }
    const PlayerParameters &parameters() override { return parameters_; }
    PlayerOrientation &orientation() override { return actor_.orientation; }
    std::int32_t pitch(bool approaching) override {
        return need(contact_.angle_x_at_34, approaching ? 0x445063 : 0x44508e);
    }
    std::int32_t roll(bool approaching) override {
        return need(contact_.angle_z_at_38, approaching ? 0x445074 : 0x445094);
    }
    void world_to_player(NinjaVector &value) override {
        try {
            result_.coordinate =
                coordinates_.world_to_player(actor_.orientation, value, arithmetic);
            if (result_.coordinate)
                return;
            result_.nested_source_boundary = result_.coordinate.source_boundary;
        } catch (const CoordinateError &error) {
            result_.coordinate.failure = error.failure();
        }
        result_.failure = ScenePositionFailure::coordinate_failed;
        result_.source_boundary = 0x444f7e;
        throw Stopped{};
    }
};
} // namespace

PlayerScenePositionResult nudge_scene_normal(PlayerTaskState &actor, PlayerMotionState *motion,
                                             const PlayerContactWork &contact,
                                             const PlayerScenePositionConstants &constants,
                                             X87Context &fp) {
    PlayerScenePositionResult result;
    const auto tuning = motion_constants(constants);
    ReferenceNudge context(actor, motion, contact, tuning, fp);
    invoke(result, [&] { scene::nudge_scene_normal(context); });
    return result;
}

PlayerScenePositionResult move_scene_platform(PlayerTaskState &actor, PlayerPhysicsState &player,
                                              PlayerContactWork &contact, const NinjaVector &offset,
                                              const ScenePlatformRuntime &runtime,
                                              const CoordinateAxisConstants &axis, X87Context &fp) {
    PlayerScenePositionResult result;
    scene::MotionConstants tuning;
    tuning.zero = axis.zero;
    ReferencePlatform context(actor, player, contact, runtime, tuning, fp, result);
    invoke(result, [&] { scene::move_scene_platform(context, offset); });
    return result;
}

PlayerScenePositionResult rotate_scene_floor(PlayerTaskState &actor, PlayerPhysicsState &player,
                                             const PlayerParameters &parameters,
                                             const PlayerContactWork &contact,
                                             const PlayerScenePositionConstants &constants,
                                             const std::optional<NinjaVector> &gravity,
                                             PlayerCoordinates &coordinates, X87Context &fp) {
    PlayerScenePositionResult result;
    const auto tuning = motion_constants(constants);
    ReferenceFloor context(actor, player, parameters, contact, gravity, coordinates, tuning, fp,
                           result);
    invoke(result, [&] { scene::rotate_scene_floor(context); });
    return result;
}
} // namespace sadx

namespace sadx {
namespace {
std::uint32_t boundary(scene::Access calculation) {
    switch (calculation) {
    case scene::Access::WallSlideSpeedGateStatus:
        return 0x43dcac;
    case scene::Access::WallSlideCrossSurfaceNormal:
        return 0x43dcd2;
    case scene::Access::WallSlideFallbackDirection:
        return 0x43dd06;
    case scene::Access::WallSlidePlanePoint:
        return 0x43dd2e;
    case scene::Access::WallSlidePlaneNormal:
        return 0x43dd3b;
    case scene::Access::WallSlidePlaneCenter:
        return 0x43dd4f;
    case scene::Access::WallSlideVelocitySlideVector:
        return 0x43de0f;
    case scene::Access::WallSlideGroundDisplacement:
        return 0x43de1b;
    case scene::Access::WallSlideRegisteredVelocityPriorAttributes:
        return 0x43de3e;
    case scene::Access::WallSlideRegisteredVelocityRegisteredVelocity:
        return 0x43de53;
    case scene::Access::WallSlideTurnDisplacement:
        return 0x43de7d;
    case scene::Access::WallSlideTurnDirection:
        return 0x43deec;
    case scene::Access::WallSlideTurnAngle:
        return 0x43defa;
    case scene::Access::WallSlideTurnRegisteredYaw:
        return 0x43df21;
    case scene::Access::CornerSlideStatus:
        return 0x43df6a;
    case scene::Access::CornerSlideSurfaceNormal:
        return 0x43df94;
    case scene::Access::CornerSlideDisplacement:
        return 0x43dfad;
    case scene::Access::CornerSlideSlideVector:
        return 0x43dfc3;
    case scene::Access::CornerSlideTurnDirection:
        return 0x43e00f;
    case scene::Access::CornerSlideTurnAngle:
        return 0x43e01d;
    case scene::Access::CornerSlideTargetYaw:
        return 0x43e038;
    case scene::Access::CornerSlideRegisteredVelocity:
        return 0x43e069;
    case scene::Access::WallWorldDirectionWorld:
        return 0x444bd4;
    case scene::Access::WallPositionDirectionPosition:
        return 0x444d86;
    case scene::Access::WallPositionCommitCorrectedPlane:
        return 0x444cf4;
    case scene::Access::WallPositionCommitPosition:
        return 0x444d0f;
    case scene::Access::WallPositionCommitSlideVector:
        return 0x444d2a;
    case scene::Access::WallResponseSurfaceNormal:
        return 0x444ab0;
    case scene::Access::GroundedWallResponseAngle:
        return 0x444b1b;
    case scene::Access::WallResponseMotionOverride:
        return 0x444b9f;
    case scene::Access::WallResponseTaskToWorld:
        return 0x444bc6;
    case scene::Access::AirborneWallResponseAngle:
        return 0x444c1d;
    case scene::Access::CornerResponseSurfaceNormal:
        return 0x444d61;
    case scene::Access::GroundedCornerResponseAngle:
        return 0x444e02;
    case scene::Access::AirborneCornerResponseAngle:
        return 0x444e69;
    case scene::Access::ContactOffsetDirectionCalculateVector:
        return 0x445dbb;
    case scene::Access::ContactOffsetHalfHeightCalculateVector:
        return 0x445df2;
    case scene::Access::AlternateContactOffsetActorInitializeOffset:
        return 0x445e03;
    case scene::Access::ContactOffsetCenterCalculateVector:
        return 0x445e16;
    case scene::Access::ContactOffsetDirectionFloorToWorld:
        return 0x4495a2;
    case scene::Access::ContactOffsetHalfHeightFloorToWorld:
        return 0x4495b1;
    case scene::Access::ContactOffsetDirectionTaskToWorld:
        return 0x4495e0;
    case scene::Access::ContactOffsetHalfHeightTaskToWorld:
        return 0x4495eb;
    case scene::Access::ContactOffsetFloorInitializeOffset:
        return 0x4495c2;
    case scene::Access::OrdinaryContactOffsetActorInitializeOffset:
        return 0x449603;
    case scene::Access::ContactOffsetCenterFloorToWorld:
        return 0x4495d4;
    case scene::Access::ContactOffsetCenterTaskToWorld:
        return 0x44960e;
    case scene::Access::MovePositionPosition:
        return 0x4497e7;
    case scene::Access::MovePositionWorld:
        return 0x4497ea;
    case scene::Access::MovePositionDisplacement:
        return 0x44a77c;
    case scene::Access::AlternateCandidateAdmissionEntryAttributes:
        return 0x445ebe;
    case scene::Access::OrdinaryCandidateAdmissionEntryAttributes:
        return 0x4496d3;
    case scene::Access::AlternateCandidateAdmissionEntryTask:
        return 0x445ed5;
    case scene::Access::OrdinaryCandidateAdmissionEntryTask:
        return 0x4496e7;
    case scene::Access::AlternateCandidateAdmissionGeometryTask:
        return 0x445edd;
    case scene::Access::OrdinaryCandidateAdmissionGeometryTask:
        return 0x4496ee;
    case scene::Access::CandidateAdmissionNullGeometryPrimary:
        return 0x445ee3;
    case scene::Access::CandidateAdmissionGeometryFlags:
        return 0x4496f7;
    case scene::Access::AlternateFirstPassQueryQuery:
        return 0x445f0a;
    case scene::Access::OrdinaryFirstPassQueryQuery:
        return 0x449710;
    case scene::Access::AlternateFirstPassQueryStatus:
        return 0x445f1c;
    case scene::Access::OrdinaryFirstPassQueryStatus:
        return 0x449723;
    case scene::Access::AlternateFirstPassGroundedResponseResponse:
        return 0x445f39;
    case scene::Access::OrdinaryFirstPassGroundedResponseResponse:
        return 0x449740;
    case scene::Access::AlternateFirstPassAirborneResponseResponse:
        return 0x445f46;
    case scene::Access::OrdinaryFirstPassAirborneResponseResponse:
        return 0x449762;
    case scene::Access::AlternateFirstPassCapacityLocalBufferOverflow:
        return 0x445f66;
    case scene::Access::OrdinaryFirstPassCapacityLocalBufferOverflow:
        return 0x44978b;
    case scene::Access::AlternateCandidateOrderMovingTask:
        return 0x446028;
    case scene::Access::OrdinaryCandidateOrderMovingTask:
        return 0x44987a;
    case scene::Access::AlternateCandidateOrderEntry:
        return 0x44604d;
    case scene::Access::OrdinaryCandidateOrderEntry:
        return 0x4498b0;
    case scene::Access::AlternateCandidateOrderCachedObject:
        return 0x446076;
    case scene::Access::OrdinaryCandidateOrderCachedObject:
        return 0x4498d6;
    case scene::Access::AlternateCandidateOrderLocalBufferOverflow:
        return 0x4460ab;
    case scene::Access::OrdinaryCandidateOrderLocalBufferOverflow:
        return 0x449914;
    case scene::Access::AlternateAirborneTailStatus:
        return 0x4467ca;
    case scene::Access::OrdinaryAirborneTailStatus:
        return 0x44a6a0;
    case scene::Access::AlternateAirborneTailInput:
        return 0x4467f5;
    case scene::Access::OrdinaryAirborneTailInput:
        return 0x44a6d2;
    case scene::Access::AlternateAirborneTailNoControlTime:
        return 0x446812;
    case scene::Access::OrdinaryAirborneTailNoControlTime:
        return 0x44a6f3;
    case scene::Access::AirborneTailPriorAttributes:
        return 0x44a71d;
    case scene::Access::AlternateAirborneTailPosition:
        return 0x446858;
    case scene::Access::OrdinaryAirborneTailPosition:
        return 0x44a743;
    case scene::Access::AlternateAirborneTailWorld:
        return 0x446852;
    case scene::Access::OrdinaryAirborneTailWorld:
        return 0x44a73a;
    case scene::Access::AlternateAirborneTailAttributes:
        return 0x4468b9;
    case scene::Access::OrdinaryAirborneTailAttributes:
        return 0x44a7ab;
    case scene::Access::ContactGateRegisteredMotion:
        return 0x444661;
    case scene::Access::ContactGateRegisteredPlayer:
        return 0x444668;
    case scene::Access::ContactGateCenterSpecialSurface:
        return 0x44467a;
    case scene::Access::ContactGateCenterAmyCenterOffset:
        return 0x4446a1;
    case scene::Access::ContactGateRegisteredParameters:
        return 0x44465b;
    case scene::Access::ContactGateCenterInitializeOffset:
        return 0x4446b4;
    case scene::Access::ContactGateCenterSurfaceNormal:
        return 0x4446e3;
    case scene::Access::ContactGateCenterFloorToWorld:
        return 0x4446e7;
    case scene::Access::ContactGateCenterTaskToWorld:
        return 0x4446f7;
    case scene::Access::ContactGateRadiusRadius:
        return 0x444705;
    case scene::Access::ContactGateRadiusAmyRadiusOffset:
        return 0x444723;
    case scene::Access::ContactGateRadiusFloorToWorld:
        return 0x44474a;
    case scene::Access::ContactGateRadiusTaskToWorld:
        return 0x44475a;
    case scene::Access::ContactGateSweepEntryAttributes:
        return 0x444785;
    case scene::Access::ContactGateSweepCenter:
        return 0x4447da;
    case scene::Access::ContactGateSweepCorrectedPlane:
        return 0x4447c6;
    case scene::Access::ContactGateSweepSlideVector:
        return 0x44478c;
    case scene::Access::ContactGateSlopeDirection:
        return 0x4447d2;
    case scene::Access::ContactGateSlopeSurfaceNormal:
        return 0x4447ce;
    case scene::Access::ContactGateSlopeAngle:
        return 0x4447f9;
    case scene::Access::ContactGateSlopeSlideVector:
        return 0x44480c;
    case scene::Access::ContactGateSlopeDisplacement:
        return 0x44484f;
    case scene::Access::ContactGateUnalignedCenterResponsePlane:
        return 0x44488d;
    case scene::Access::ContactGateUnalignedCenterCorrectedPlane:
        return 0x444895;
    case scene::Access::ContactGateUnalignedCenterPosition:
        return 0x4448a4;
    case scene::Access::ContactGateRadiusRegisteredAttributes:
        return 0x444901;
    case scene::Access::ContactGateRadiusRenderMode:
        return 0x444967;
    case scene::Access::ContactGateCallAgainAgainAgain:
        return 0x44499e;
    case scene::Access::ContactGateQueryQuery:
        return 0x4449e3;
    case scene::Access::ContactGateQueryDirection:
        return 0x4449f4;
    case scene::Access::ContactGateQuerySurfaceNormal:
        return 0x4449f1;
    case scene::Access::ContactGateQueryAngle:
        return 0x4449f9;
    case scene::Access::ContactGateQueryGravity:
        return 0x444a0a;
    case scene::Access::ContactGateAcceptGravity:
        return 0x444a3b;
    case scene::Access::ContactGateAcceptRegisteredStatus:
        return 0x444a56;
    case scene::Access::AlternateFootProjectionPlane:
        return 0x4465bd;
    case scene::Access::OrdinaryFootProjectionSlide:
        return 0x44a37a;
    case scene::Access::OrdinaryFootProjectionPlane:
        return 0x44a37d;
    case scene::Access::AlternateFootProjectionCenter:
        return 0x4465cc;
    case scene::Access::OrdinaryFootProjectionCenter:
        return 0x44a38a;
    case scene::Access::FootProjectionSelectedContact:
        return 0x4465c0;
    case scene::Access::AlternateFootProjectionSelectedResponse:
        return 0x446604;
    case scene::Access::OrdinaryFootProjectionSelectedResponse:
        return 0x44a3c6;
    case scene::Access::AlternateFootProjectionDirection:
        return 0x4465c6;
    case scene::Access::OrdinaryFootProjectionDirection:
        return 0x44a384;
    case scene::Access::FootProjectionDistance:
        return 0x44663b;
    case scene::Access::FootProjectionPlaneProjection:
        return 0x44a410;
    case scene::Access::AlternateFootProjectionGeometryTask:
        return 0x44666c;
    case scene::Access::OrdinaryFootProjectionGeometryTask:
        return 0x44a43d;
    case scene::Access::AlternateFootProjectionMovePlatform:
        return 0x44666f;
    case scene::Access::OrdinaryFootProjectionMovePlatform:
        return 0x44a447;
    case scene::Access::AlternateFootProjectionPosition:
        return 0x4466b0;
    case scene::Access::OrdinaryFootProjectionPosition:
        return 0x44a481;
    case scene::Access::FootProjectionMotionOverride:
        return 0x44a4b7;
    case scene::Access::AlternateInitialStatus:
        return 0x445d1d;
    case scene::Access::OrdinaryInitialStatus:
        return 0x44950d;
    case scene::Access::AlternateInitialBindContact:
        return 0x445d2e;
    case scene::Access::OrdinaryInitialBindContact:
        return 0x44951c;
    case scene::Access::InitialPriorAttributes:
        return 0x44958d;
    case scene::Access::InitialFloorNormal:
        return 0x44959e;
    case scene::Access::AlternateInitialSweepCenter:
        return 0x445e25;
    case scene::Access::OrdinaryInitialSweepCenter:
        return 0x449616;
    case scene::Access::AlternateInitialPosePosition:
        return 0x445e75;
    case scene::Access::OrdinaryInitialPosePosition:
        return 0x449674;
    case scene::Access::AlternateInitialBroadphase:
        return 0x445e89;
    case scene::Access::OrdinaryInitialBroadphase:
        return 0x449685;
    case scene::Access::AlternateEmptyLandscapeAttributes:
        return 0x445fda;
    case scene::Access::OrdinaryEmptyLandscapeAttributes:
        return 0x44980b;
    case scene::Access::AlternateContactSweepCenter:
        return 0x446164;
    case scene::Access::AlternateContactResponseEntry:
        return 0x4461d3;
    case scene::Access::AlternateContactResponseQuery:
        return 0x4461ea;
    case scene::Access::AlternateContactResponseEntryAttributes:
        return 0x44621d;
    case scene::Access::AlternateContactResponseSlopeSurfaceNormal:
        return 0x4461fc;
    case scene::Access::AlternateContactResponseSlopeDirection:
        return 0x4461ff;
    case scene::Access::AlternateContactResponseSlopeAngle:
        return 0x446204;
    case scene::Access::ContactResponseEnvironment:
        return 0x446231;
    case scene::Access::AlternateContactResponseStatus:
        return 0x446279;
    case scene::Access::AlternateContactGravityGateGravity:
        return 0x446285;
    case scene::Access::AlternateContactResponseEntryObject:
        return 0x44630e;
    case scene::Access::AlternateContactResponseResponse:
        return 0x446323;
    case scene::Access::AlternateFloorTurnStoreFloorAngles:
        return 0x44635d;
    case scene::Access::AlternateFloorTurnCheckWorld:
        return 0x446377;
    case scene::Access::AlternateFloorTurnCheckStatus:
        return 0x44638a;
    case scene::Access::AlternateFloorTurnCheckGravity:
        return 0x446394;
    case scene::Access::AlternateFloorTurnRotateFloor:
        return 0x4463cb;
    case scene::Access::AlternateFloorMotionStoresWorld:
        return 0x4464d4;
    case scene::Access::AlternateFloorMotionStoresContactPitch:
        return 0x4464ce;
    case scene::Access::AlternateFloorMotionStoresContactRoll:
        return 0x4464da;
    case scene::Access::AlternateFloorMotionStoresSurfaceNormal:
        return 0x4464e0;
    case scene::Access::AlternateSlideClampSlideVector:
        return 0x4464f4;
    case scene::Access::AlternateSlideClampDisplacement:
        return 0x44651a;
    case scene::Access::AlternateContactResponseCommitEntryTask:
        return 0x446657;
    case scene::Access::ContactResponseCommitEntryAttributes:
        return 0x44674f;
    case scene::Access::ContactResponseCommitAttributes:
        return 0x44675d;
    case scene::Access::FloorTurnCheckWorld:
        return 0x44a02f;
    case scene::Access::FloorTurnCheckStatus:
        return 0x44a056;
    case scene::Access::FloorTurnCheckGravity:
        return 0x44a05e;
    case scene::Access::FloorTurnCheckSurfaceNormal:
        return 0x44a063;
    case scene::Access::FloorTurnCheckDirection:
        return 0x44a06c;
    case scene::Access::FloorTurnSurfaceNormal:
        return 0x449fbf;
    case scene::Access::FloorTurnStatus:
        return 0x449fac;
    case scene::Access::OrdinaryFloorTurnStoreFloorAngles:
        return 0x44a14b;
    case scene::Access::FloorTurnPitchAsin:
        return 0x449fc2;
    case scene::Access::FloorTurnInvertedRollAsin:
        return 0x44a008;
    case scene::Access::FloorTurnRollAsin:
        return 0x449fed;
    case scene::Access::FloorTurnReflectedPitchAsin:
        return 0x44a08b;
    case scene::Access::FloorTurnReflectedRollAsin:
        return 0x44a0a7;
    case scene::Access::OrdinaryFloorTurnRotateFloor:
        return 0x44a1c0;
    case scene::Access::LandingGravity:
        return 0x44a54d;
    case scene::Access::LandingDirection:
        return 0x44a548;
    case scene::Access::LandingAirborneAttributes:
        return 0x44a5f2;
    case scene::Access::LandingAirborneNudgeNormal:
        return 0x44a60d;
    case scene::Access::LandingGroundedAttributes:
        return 0x44a5cf;
    case scene::Access::ContactResponseEventTask:
        return 0x42fb00;
    case scene::Access::OrdinaryContactSweepCenter:
        return 0x4499b2;
    case scene::Access::OrdinaryContactResponseEntry:
        return 0x449a10;
    case scene::Access::OrdinaryContactResponseQuery:
        return 0x449a3e;
    case scene::Access::OrdinaryContactResponseEntryAttributes:
        return 0x449da9;
    case scene::Access::OrdinaryContactResponseEntryObject:
        return 0x449a6d;
    case scene::Access::ContactResponseSpecialSurface:
        return 0x449a76;
    case scene::Access::ContactDeltaSlideVector:
        return 0x449a7c;
    case scene::Access::ContactDeltaResponsePlane:
        return 0x449a83;
    case scene::Access::ContactDeltaPosition:
        return 0x449a86;
    case scene::Access::ContactDeltaSurfaceNormal:
        return 0x449b59;
    case scene::Access::ContactForwardDeltaSlideVector:
        return 0x449bb9;
    case scene::Access::ContactForwardDeltaResponsePlane:
        return 0x449bc3;
    case scene::Access::ContactForwardDeltaCenter:
        return 0x449bc6;
    case scene::Access::ContactForwardDeltaTaskToWorld:
        return 0x449cb3;
    case scene::Access::OrdinaryContactResponseSlopeDirection:
        return 0x449d68;
    case scene::Access::OrdinaryContactResponseSlopeSurfaceNormal:
        return 0x449d65;
    case scene::Access::OrdinaryContactResponseSlopeAngle:
        return 0x449d6d;
    case scene::Access::ContactResponseRecheckAngle:
        return 0x449d8c;
    case scene::Access::ContactSlopeGravity:
        return 0x449dcb;
    case scene::Access::OrdinaryContactResponseStatus:
        return 0x449e18;
    case scene::Access::ContactCeilingGravity:
        return 0x449e24;
    case scene::Access::ContactResponseAction:
        return 0x449e6d;
    case scene::Access::ContactResponseWorld:
        return 0x449e7d;
    case scene::Access::OrdinaryContactResponseResponse:
        return 0x449f5d;
    case scene::Access::ContactOffsetFloorSurfaceNormal:
        return 0x44a204;
    case scene::Access::OrdinaryFloorMotionStoresWorld:
        return 0x44a2da;
    case scene::Access::OrdinaryFloorMotionStoresContactPitch:
        return 0x44a2d7;
    case scene::Access::OrdinaryFloorMotionStoresContactRoll:
        return 0x44a2e4;
    case scene::Access::OrdinaryFloorMotionStoresSurfaceNormal:
        return 0x44a2f1;
    case scene::Access::FloorSlideClampSlideVector:
        return 0x44a313;
    case scene::Access::FloorSlideClampDisplacement:
        return 0x44a33f;
    case scene::Access::OrdinaryContactResponseCommitEntryTask:
        return 0x44a432;
    case scene::Access::Known:
        return 0;
    }
    throw std::logic_error("unknown scene calculation");
}

scene::Constants dispatch_constants(const PlayerScenePositionConstants &constants) {
    scene::Constants value;
    value.wall_alignment = constants.single(0x7dcc74);
    value.push_distance = constants.single(0x7dccd8);
    value.big_radius_addition = constants.single(0x7dd540);
    value.corner_speed_minimum = constants.single(0x7dd550);
    value.contact_delta_alignment = constants.single(0x7dd570);
    value.backstep_alignment = constants.single(0x7dd578);
    value.corner_fast_speed = constants.single(0x7dd584);
    value.turn_direction_minimum = constants.single(0x7dd58c);
    value.active_input_minimum = constants.single(0x7dfff0);
    value.wall_axis_minimum = constants.single(0x7e0084);
    value.corner_speed_multiplier = constants.single(0x7e0088);
    value.foot_position_minimum = constants.single(0x7e008c);
    value.forward_contact_alignment = constants.single(0x7e00e4);
    value.alternate_wall_normal_y = constants.single(0x7e0118);
    value.horizontal_contact_limit = constants.single(0x7e0128);
    value.wall_stop_speed = constants.single(0x7e012c);
    value.landing_lower_speed = constants.single(0x7e0174);
    value.landing_upper_speed = constants.single(0x7e0178);
    value.landing_gravity_alignment = constants.single(0x7e017c);
    value.landing_reverse_speed = constants.single(0x7e0180);
    value.landing_forward_speed = constants.single(0x7e0184);
    value.fast_wall_alignment = constants.single(0x7e0188);
    value.positive_angle_factor = constants.wide(0x7dccc8);
    value.angle_units = constants.wide(0x7dccd0);
    value.negative_angle_factor = constants.wide(0x7dcce8);
    return value;
}
// NOTSA: reference-only fact availability and native callee composition.
struct ReferenceDispatcher final : scene::Context {
    ReferenceDispatcher(PlayerTaskState &actor, PlayerMotionState *world,
                        PlayerPhysicsState &player, PlayerParameters &parameters,
                        PlayerSceneLandscape &landscape,
                        const PlayerScenePositionConstants &constants,
                        PlayerScenePositionRuntime &runtime, PlayerCoordinates &coordinates,
                        X87Context &fp, const scene::Constants &tuning)
        : Context(actor.orientation, player.motion, parameters, actor.player_index,
                  actor.character_id, player.dotp, tuning, coordinates, fp, runtime.axis_constants),
          actor(actor), world(world), player(player), landscape(landscape), constants(constants),
          runtime(runtime) {}
    PlayerTaskState &actor;
    PlayerMotionState *world;
    PlayerPhysicsState &player;
    PlayerSceneLandscape &landscape;
    const PlayerScenePositionConstants &constants;
    PlayerScenePositionRuntime &runtime;
    PlayerScenePositionResult report;
    PlayerContactWork *contact = nullptr;
    PlayerContactWork backup;
    std::shared_ptr<PlayerStartupPlayer> registered_allocation;
    PlayerPhysicsState *registered_player = nullptr;
    PlayerParameters *registered_parameters_owner = nullptr;
    template <class T> T &read(std::optional<T> &value, scene::Access calculation) {
        if (calculation == scene::Access::Known)
            return *value;
        return need(value, boundary(calculation));
    }
    template <class T>
    const T &read(const std::optional<T> &value, scene::Access calculation) {
        if (calculation == scene::Access::Known)
            return *value;
        return need(value, boundary(calculation));
    }
    [[noreturn]] void stop(ScenePositionFailure failure, std::uint32_t site,
                           std::uint32_t nested = 0) {
        report.failure = failure;
        report.source_boundary = site;
        report.nested_source_boundary = nested;
        throw Stopped{};
    }
    void coordinate(CoordinateResult value, scene::Access calculation) {
        report.coordinate = value;
        if (!value)
            stop(ScenePositionFailure::coordinate_failed, boundary(calculation),
                 value.source_boundary);
    }
    void leaf(PlayerScenePositionResult value, scene::Access calculation) {
        if (!value) {
            report.platform_callback = std::make_shared<PlayerScenePositionResult>(value);
            stop(value.failure, boundary(calculation), value.source_boundary);
        }
    }
    NinjaVector &position(scene::Access calculation) override {
        return read(actor.position, calculation);
    }
    void position(const NinjaVector &value) override { actor.position = value; }
    std::uint8_t &action(scene::Access calculation) override {
        return read(actor.action_at_0, calculation);
    }
    std::uint16_t &status(scene::Access calculation) override {
        return read(player.flags_at_10, calculation);
    }
    void status(const std::uint16_t &value) override { player.flags_at_10 = value; }
    std::uint32_t &prior_attributes(scene::Access calculation) override {
        return read(player.prior_attributes, calculation);
    }
    NinjaVector &floor_normal(scene::Access calculation) override {
        return read(player.floor_normal, calculation);
    }
    void floor_normal(const NinjaVector &value) override { player.floor_normal = value; }
    TaskHandle &moving_task(scene::Access calculation) override {
        return read(player.moving_task_at_6c, calculation);
    }
    void moving_task(const TaskHandle &value) override { player.moving_task_at_6c = value; }
    SceneResourceReference &cached_object(scene::Access calculation) override {
        return read(player.cached_land_object_at_74, calculation);
    }
    void cached_object(const SceneResourceReference &value) override {
        player.cached_land_object_at_74 = value;
    }
    std::int16_t &no_control_time(scene::Access calculation) override {
        return read(player.no_control_time_at_a, calculation);
    }
    NinjaVector &collision_normal(scene::Access calculation) override {
        return read(player.source_vector_at_44, calculation);
    }
    void collision_normal(const NinjaVector &value) override { player.source_vector_at_44 = value; }
    std::uint32_t &special_surface(scene::Access calculation) override {
        return read(runtime.globals.special_surface_at_3b42e74, calculation);
    }
    void special_surface(const std::uint32_t &value) override {
        runtime.globals.special_surface_at_3b42e74 = value;
    }
    std::uint32_t &motion_override(scene::Access calculation) override {
        return read(runtime.globals.motion_override_at_3b42e70, calculation);
    }
    TaskHandle &event_task(scene::Access calculation) override {
        return read(runtime.globals.event_task_at_3b2c578, calculation);
    }
    std::uint8_t &environment(scene::Access calculation) override {
        return read(runtime.globals.environment_at_3b22dec, calculation);
    }
    std::int8_t &render_mode(scene::Access calculation) override {
        return read(runtime.globals.render_at_3b0f0ec, calculation);
    }
    float &amy_center_offset(scene::Access calculation) override {
        return read(runtime.globals.amy_offset_at_915d74, calculation);
    }
    float &amy_radius_offset(scene::Access calculation) override {
        return read(runtime.globals.amy_radius_at_915d78, calculation);
    }
    const NinjaVector &gravity(scene::Access calculation) override {
        return read(runtime.response_math.world_gravity, calculation);
    }
    float &radius(scene::Access calculation) override {
        return read(contact->radius_at_0, calculation);
    }
    void radius(const float &value) override { contact->radius_at_0 = value; }
    NinjaVector &center(scene::Access calculation) override {
        return read(contact->center_at_4, calculation);
    }
    void center(const NinjaVector &value) override { contact->center_at_4 = value; }
    NinjaVector &displacement(scene::Access calculation) override {
        return read(contact->displacement_at_10, calculation);
    }
    void displacement(const NinjaVector &value) override { contact->displacement_at_10 = value; }
    NinjaVector &direction(scene::Access calculation) override {
        return read(contact->direction_at_1c, calculation);
    }
    void direction(const NinjaVector &value) override { contact->direction_at_1c = value; }
    std::int32_t &contact_pitch(scene::Access calculation) override {
        return read(contact->angle_x_at_34, calculation);
    }
    void contact_pitch(const std::int32_t &value) override { contact->angle_x_at_34 = value; }
    std::int32_t &contact_roll(scene::Access calculation) override {
        return read(contact->angle_z_at_38, calculation);
    }
    void contact_roll(const std::int32_t &value) override { contact->angle_z_at_38 = value; }
    NinjaVector &response_normal(scene::Access calculation) override {
        return read(contact->vector_at_3c, calculation);
    }
    void response_normal(const NinjaVector &value) override { contact->vector_at_3c = value; }
    NinjaVector &surface_normal(scene::Access calculation) override {
        return read(contact->vector_at_48, calculation);
    }
    void surface_normal(const NinjaVector &value) override { contact->vector_at_48 = value; }
    NinjaVector &response_plane(scene::Access calculation) override {
        return read(contact->vector_at_54, calculation);
    }
    void response_plane(const NinjaVector &value) override { contact->vector_at_54 = value; }
    NinjaVector &corrected_plane(scene::Access calculation) override {
        return read(contact->vector_at_60, calculation);
    }
    void corrected_plane(const NinjaVector &value) override { contact->vector_at_60 = value; }
    NinjaVector &slide_vector(scene::Access calculation) override {
        return read(contact->vector_at_6c, calculation);
    }
    void slide_vector(const NinjaVector &value) override { contact->vector_at_6c = value; }
    std::uint16_t &selected_contact(scene::Access calculation) override {
        return read(contact->selected_contact_at_2a, calculation);
    }
    std::uint32_t &registered_attributes(scene::Access calculation) override {
        return read(registered_player->prior_attributes, calculation);
    }
    std::uint16_t &registered_status(scene::Access calculation) override {
        return read(registered_player->flags_at_10, calculation);
    }

    void remember_flags(std::uint16_t value) override {
        Context::remember_flags(value);
        report.previous_flags = value;
    }
    void remember_position(const NinjaVector &value) override {
        Context::remember_position(value);
        report.temporary_vector = value;
    }
    void remember_offsets(const NinjaVector &half, const NinjaVector &center) override {
        Context::remember_offsets(half, center);
        report.half_height_offset = half;
        report.center_offset = center;
    }
    void bind_contact(scene::Access calculation) override {
        if (!player.contact_work)
            stop(ScenePositionFailure::unavailable_contact_work, boundary(calculation));
        contact = &*player.contact_work;
    }
    void require_world(scene::Access calculation) override {
        if (!world)
            stop(ScenePositionFailure::null_motion, boundary(calculation));
    }
    NinjaVector &world_velocity(scene::Access calculation) override {
        require_world(calculation);
        return world->motion.speed;
    }
    void world_pitch(std::int32_t value) override { world->angle_x_at_1c = value; }
    void world_roll(std::int32_t value) override { world->angle_z_at_24 = value; }
    PlayerMotionState &registered_world(scene::Access calculation) {
        if (!runtime.platform.players ||
            actor.player_index >= runtime.platform.players->motion_work.size())
            stop(ScenePositionFailure::unavailable_registry_owner, boundary(calculation));
        const auto &selected =
            read(runtime.platform.players->motion_work[actor.player_index], calculation);
        if (!selected)
            stop(ScenePositionFailure::null_motion, boundary(calculation));
        return *selected;
    }
    NinjaVector &registered_velocity(scene::Access calculation) override {
        return registered_world(calculation).motion.speed;
    }
    std::int32_t &registered_yaw(scene::Access calculation) override {
        return registered_world(calculation).target_yaw;
    }
    void select_registered_player() override {
        const auto registry = runtime.platform.players;
        if (!registry || actor.player_index >= registry->player_work.size())
            stop(ScenePositionFailure::unavailable_registry_owner, 0x444661);
        registered_allocation = need(registry->player_work[actor.player_index], 0x444661);
        if (!registered_allocation || !registered_allocation->player ||
            !registered_allocation->parameters)
            stop(ScenePositionFailure::unavailable_registry_owner, 0x444668);
        registered_player = registered_allocation->player.get();
        registered_parameters_owner = &*registered_allocation->parameters;
    }
    const PlayerParameters &registered_parameters() override {
        return *registered_parameters_owner;
    }
    NinjaVector &registered_player_speed() override { return registered_player->motion.speed; }
    void save_contact() override { backup = *contact; }
    void restore_contact() override { *contact = backup; }
    void restore_radius() override { contact->radius_at_0 = backup.radius_at_0; }
    void restore_displacement() override {
        contact->displacement_at_10 = backup.displacement_at_10;
    }
    std::int32_t candidate_count() override { return landscape.candidate_count; }
    std::optional<SceneLandEntry> &slot(scene::EntrySlot entry) {
        switch (entry.collection) {
        case scene::EntryCollection::Candidates:
            return landscape.candidates.at(entry.index);
        case scene::EntryCollection::FirstPass:
            return report.first_pass.at(entry.index);
        case scene::EntryCollection::Ordered:
            return report.ordered.at(entry.index);
        }
        throw std::logic_error("unknown entry collection");
    }
    scene::EntrySlot entry(scene::EntryCollection collection, std::size_t index,
                           scene::Access calculation) override {
        scene::EntrySlot entry{collection, index};
        (void)read(slot(entry), calculation);
        return entry;
    }
    std::uint32_t entry_attributes(scene::EntrySlot entry,
                                   scene::Access calculation) override {
        return read(read(slot(entry), scene::Access::Known).attributes, calculation);
    }
    SceneResourceReference entry_object(scene::EntrySlot entry,
                                        scene::Access calculation) override {
        return read(read(slot(entry), scene::Access::Known).object, calculation);
    }
    SceneResourceReference entry_task(scene::EntrySlot entry,
                                      scene::Access calculation) override {
        return read(read(slot(entry), scene::Access::Known).task, calculation);
    }
    void copy_entry(scene::EntrySlot source, scene::EntrySlot destination) override {
        slot(destination) = slot(source);
    }
    void swap_ordered(std::size_t a, std::size_t b) override {
        std::swap(report.ordered[a], report.ordered[b]);
    }
    TaskHandle geometry_task(const SceneResourceReference &ref,
                             scene::Access calculation) override {
        if (!ref.id)
            return {};
        if (!runtime.geometry_tasks)
            stop(ScenePositionFailure::unavailable_geometry_task, boundary(calculation));
        const auto found = runtime.geometry_tasks->find(*ref.id);
        if (found == runtime.geometry_tasks->end() || !found->second)
            stop(ScenePositionFailure::unavailable_geometry_task, boundary(calculation));
        return found->second;
    }
    bool geometry_has_primary(const TaskHandle &task) override {
        return static_cast<bool>(task->primary_work);
    }
    std::uint16_t geometry_flags(const TaskHandle &task, scene::Access calculation) override {
        if (!task->primary_state)
            stop(ScenePositionFailure::unavailable_live_primary, boundary(calculation));
        return task->primary_state->orientation.flags;
    }
    void broadphase(scene::Access calculation) override {
        report.broadphase = query_player_scene_broadphase(
            landscape, *contact, runtime.broadphase_constants, coordinates, fp);
        if (!*report.broadphase)
            stop(ScenePositionFailure::broadphase_failed, boundary(calculation),
                 report.broadphase->source_boundary);
    }
    bool query(scene::EntrySlot entry, scene::Access calculation) override {
        const auto object = entry_object(entry, calculation);
        report.mesh_query = query_mesh_object_contacts(
            *contact, landscape, object, runtime.contacts, runtime.query_constants,
            runtime.floor_constants, runtime.axis_constants, runtime.query_math, coordinates, fp);
        if (!*report.mesh_query)
            stop(ScenePositionFailure::mesh_query_failed, boundary(calculation),
                 report.mesh_query->source_boundary);
        return *report.mesh_query->outcome != 0;
    }
    std::int32_t respond(scene::Response response, std::uint32_t attributes,
                         scene::Access calculation) override {
        PlayerContactResponseEntry entry;
        switch (response) {
        case scene::Response::OrdinaryGrounded:
            entry = PlayerContactResponseEntry::at_43cce0;
            break;
        case scene::Response::AlternateGrounded:
            entry = PlayerContactResponseEntry::at_43cf90;
            break;
        case scene::Response::Airborne:
            entry = PlayerContactResponseEntry::at_43d180;
            break;
        case scene::Response::OrdinaryLanding:
            entry = PlayerContactResponseEntry::at_43d360;
            break;
        case scene::Response::AlternateLanding:
            entry = PlayerContactResponseEntry::at_43d8b0;
            break;
        }
        report.response = respond_player_contacts(
            entry, *contact, runtime.contacts, {&actor, world, &player, &parameters, attributes},
            runtime.response_constants, runtime.response_math, fp);
        if (!*report.response)
            stop(ScenePositionFailure::response_failed, boundary(calculation),
                 report.response->source_boundary);
        return *report.response->outcome;
    }
    void attributes(std::uint32_t incoming, scene::Access calculation) override {
        report.attributes = apply_player_contact_attributes(actor, world, player, parameters,
                                                            runtime.parameter_defaults, incoming,
                                                            runtime.attribute_constants, fp);
        if (!*report.attributes)
            stop(ScenePositionFailure::attributes_failed, boundary(calculation),
                 report.attributes->source_boundary);
    }
    std::int32_t angle(const NinjaVector &a, const NinjaVector &b, const NinjaVector *sign,
                       scene::Access calculation) override {
        report.projection = projection_vector_angle(
            a, b, sign, runtime.response_constants.plane.axis, runtime.response_constants.angle,
            runtime.response_math.inverse_math, fp);
        if (!*report.projection)
            stop(ScenePositionFailure::projection_failed, boundary(calculation),
                 report.projection->source_boundary);
        return *report.projection->angle;
    }
    void initialize_offset(NinjaVector &center, scene::ActorSelection selection,
                           scene::Access calculation) override {
        PlayerMotionState *motion = world;
        PlayerPhysicsState *selected_player = &player;
        const PlayerParameters *p = &parameters;
        if (selection == scene::ActorSelection::Registered) {
            const auto registry = runtime.platform.players;
            if (!registry || actor.player_index >= registry->motion_work.size())
                stop(ScenePositionFailure::unavailable_registry_owner, 0x44465b);
            const auto &selected = need(registry->motion_work[actor.player_index], 0x44465b);
            if (!selected)
                stop(ScenePositionFailure::null_motion, 0x44465b);
            motion = selected.get();
            selected_player = registered_player;
            p = registered_parameters_owner;
        } else
            require_world(calculation);
        report.initialization = initialize_player_contact_offset(
            center, *motion, *selected_player, *contact, *p, selected_player->flags_at_10,
            runtime.response_math.world_gravity, runtime.initializer_constants,
            runtime.rotation_constants, fp);
        if (!*report.initialization)
            stop(ScenePositionFailure::initializer_failed, boundary(calculation),
                 report.initialization->source_boundary);
    }
    void task_to_world(NinjaVector &value, scene::Access calculation) override {
        coordinate(coordinates.task_to_world(&actor.orientation, value, fp), calculation);
    }
    void calculate_vector(const NinjaMatrix &matrix, const NinjaVector &value, NinjaVector &out,
                          scene::Access calculation) override {
        coordinate(coordinates.calculate_vector(matrix, value, out, fp), calculation);
    }
    void floor_to_world(const NinjaVector &normal, NinjaVector &value,
                        scene::Access calculation) override {
        const NormalAnglesProvider owner = [&](const NinjaVector &normal, X87Context &fp) {
            report.floor_angles = floor_normal_angles(normal, runtime.floor_constants,
                                                      runtime.query_math.inverse_math, fp);
            const auto &result = *report.floor_angles;
            return NormalAnglesResult{result.failure == CrtAsinFailure::none && result.angles
                                          ? CoordinateFailure::none
                                          : CoordinateFailure::normal_angles_unavailable,
                                      result.angles};
        };
        coordinate(
            coordinates.floor_to_world(normal, value, actor.orientation.angles[1], owner, fp),
            calculation);
    }
    void intersect_planes(const NinjaPlane &first, const NinjaPlane &second,
                          NinjaPlane &out) override {
        (void)intersect_source_planes(first, second, &out, runtime.response_constants.plane, fp);
    }
    void require_selected_response(std::uint16_t selected,
                                   scene::Access calculation) override {
        if (selected >= runtime.contacts.responses.size() || !runtime.contacts.responses[selected])
            stop(ScenePositionFailure::unavailable_field, boundary(calculation));
    }
    void project_to_response_plane(const NinjaVector &center, std::uint16_t selected,
                                   const NinjaVector &normal, NinjaVector &out) override {
        (void)project_point_onto_plane(center, runtime.contacts.responses[selected]->point_at_20,
                                       normal, &out,
                                       runtime.response_constants.plane.distance_scale, fp);
    }
    void store_floor_angles(const NinjaVector &normal, scene::Access calculation) override {
        try {
            report.floor_angles = floor_normal_angles(normal, runtime.floor_constants,
                                                      runtime.query_math.inverse_math, fp);
            if (report.floor_angles->failure != CrtAsinFailure::none ||
                !report.floor_angles->angles)
                stop(ScenePositionFailure::floor_angles_failed, boundary(calculation),
                     report.floor_angles->source_boundary);
            contact->angle_x_at_34 = report.floor_angles->angles->inclination;
            contact->angle_z_at_38 = report.floor_angles->angles->roll;
        } catch (const FloorAngleArithmeticError &error) {
            contact->angle_x_at_34 = error.stored_inclination();
            throw;
        }
    }
    fidelity::ExactFloat asin(float component, scene::Access calculation) override {
        report.asin = crt_asin(fp.from_float(component), runtime.query_math.inverse_math, fp);
        if (report.asin->failure != CrtAsinFailure::none || !report.asin->value)
            stop(ScenePositionFailure::asin_failed, boundary(calculation),
                 report.asin->source_boundary);
        return fidelity::ExactFloat::retained(fp, *report.asin->value);
    }
    void rotate_floor(scene::Access calculation) override {
        leaf(sadx::rotate_scene_floor(actor, player, parameters, *contact, constants,
                                      runtime.response_math.world_gravity, coordinates, fp),
             calculation);
    }
    void nudge_normal(scene::Access calculation) override {
        leaf(sadx::nudge_scene_normal(actor, world, *contact, constants, fp), calculation);
    }
    void move_platform(const NinjaVector &offset, scene::Access calculation) override {
        leaf(sadx::move_scene_platform(actor, player, *contact, offset, runtime.platform,
                                       runtime.axis_constants, fp),
             calculation);
    }
    scene::Input read_input(scene::Access calculation) override {
        float magnitude = 0;
        report.input = get_player_input(input_task_view(actor), runtime.input, fp, nullptr,
                                        &magnitude, runtime.forced_angle);
        if (!*report.input || !report.input->active)
            stop(ScenePositionFailure::unavailable_field, boundary(calculation),
                 report.input->source_boundary);
        return {*report.input->active, magnitude};
    }
    void draw_contact_marker() override {
        stop(ScenePositionFailure::render_owner_unavailable, 0x44499e);
    }
    [[noreturn]] void fail(scene::Failure failure, scene::Access calculation) override {
        switch (failure) {
        case scene::Failure::NullGeometryPrimary:
            stop(ScenePositionFailure::null_geometry_primary, boundary(calculation));
        case scene::Failure::LocalBufferOverflow:
            stop(ScenePositionFailure::source_local_buffer_overflow, boundary(calculation));
        default:
            throw std::logic_error("unexpected native-only scene failure");
        }
    }
    void finish_report() {
        report.provisional_outcome = result.provisional_outcome;
        report.candidate_index = result.candidate_index;
        report.processed_count = result.processed_count;
        report.response_index = result.response_index;
        report.winning_index = result.winning_index;
    }
};
PlayerScenePositionResult run_dispatcher(bool alternate, PlayerTaskState &actor,
                                         PlayerMotionState *world, PlayerPhysicsState &player,
                                         PlayerParameters &parameters,
                                         PlayerSceneLandscape &landscape,
                                         const PlayerScenePositionConstants &constants,
                                         PlayerScenePositionRuntime &runtime,
                                         PlayerCoordinates &coordinates, X87Context &fp) {
    const auto tuning = dispatch_constants(constants);
    ReferenceDispatcher context(actor, world, player, parameters, landscape, constants, runtime,
                                coordinates, fp, tuning);
    try {
        context.report.outcome = alternate ? scene::position_player_in_scene_alternate(context)
                                           : scene::position_player_in_scene(context);
    } catch (Stopped) {
    } catch (Missing missing) {
        context.report.failure = missing.why;
        context.report.source_boundary = missing.site;
    } catch (const CoordinateError &error) {
        context.report.failure = ScenePositionFailure::coordinate_failed;
        context.report.coordinate.failure = error.failure();
    }
    context.finish_report();
    return context.report;
}
} // namespace
PlayerScenePositionResult position_player_in_scene(PlayerTaskState &actor, PlayerMotionState *world,
                                                   PlayerPhysicsState &player,
                                                   PlayerParameters &parameters,
                                                   PlayerSceneLandscape &landscape,
                                                   const PlayerScenePositionConstants &constants,
                                                   PlayerScenePositionRuntime &runtime,
                                                   PlayerCoordinates &coordinates, X87Context &fp) {
    return run_dispatcher(false, actor, world, player, parameters, landscape, constants, runtime,
                          coordinates, fp);
}
PlayerScenePositionResult position_player_in_scene_alternate(
    PlayerTaskState &actor, PlayerMotionState *world, PlayerPhysicsState &player,
    PlayerParameters &parameters, PlayerSceneLandscape &landscape,
    const PlayerScenePositionConstants &constants, PlayerScenePositionRuntime &runtime,
    PlayerCoordinates &coordinates, X87Context &fp) {
    return run_dispatcher(true, actor, world, player, parameters, landscape, constants, runtime,
                          coordinates, fp);
}
} // namespace sadx
