#include "guest/player_scene_position_reference.hpp"
#include "player/player_scene_position_helpers_internal.hpp"
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
