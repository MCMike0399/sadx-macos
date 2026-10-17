#include "player/player_scene_position_helpers_internal.hpp"

namespace sadx::scene {
namespace {
// NOTSA: plain borrowed owners for the shared game calculation.
struct NativeNudge final : NudgeContext {
    NativeNudge(Actor &actor, MotionWork &world, const NinjaVector &normal, X87Context &fp,
                const MotionConstants &constants)
        : NudgeContext(fp, constants), actor_(actor), world_(world), normal_(normal) {}
    Actor &actor_;
    MotionWork &world_;
    const NinjaVector &normal_;
    const NinjaVector &normal() override { return normal_; }
    NinjaVector &position() override { return actor_.position; }
    MotionWork &world_motion() override { return world_; }
};

// NOTSA: plain borrowed owners for the shared game calculation.
struct NativePlatform final : PlatformContext {
    NativePlatform(Actor &actor, const PlayerMotion &motion, NinjaVector &center,
                   PlatformWork *work, const std::array<TaskHandle, 8> &players, X87Context &fp,
                   const MotionConstants &constants)
        : PlatformContext(fp, constants), actor_(actor), motion_(motion), center_(center),
          work_(work), players_(players) {}
    Actor &actor_;
    const PlayerMotion &motion_;
    NinjaVector &center_;
    PlatformWork *work_;
    const std::array<TaskHandle, 8> &players_;
    PlatformRow *row_ = nullptr;
    bool has_force_work() override { return work_ != nullptr; }
    std::uint8_t player_index() override { return actor_.player_index; }
    void select_row(std::size_t index) override { row_ = &work_->rows[index]; }
    const NinjaVector &speed() override { return motion_.speed; }
    const NinjaVector &position(PositionUse) override { return actor_.position; }
    void position(const NinjaVector &value) override { actor_.position = value; }
    NinjaVector &center(CenterUse) override { return center_; }
    bool has_callback() override { return static_cast<bool>(row_->callback); }
    void run_callback() override {
        const auto player_task = players_.at(actor_.player_index);
        row_->callback(player_task, actor_, *row_, arithmetic);
    }
    float displacement(std::size_t index) override {
        switch (index) {
        case 0:
            return row_->displacement.x;
        case 1:
            return row_->displacement.y;
        default:
            return row_->displacement.z;
        }
    }
    std::int32_t angle_delta(std::size_t index) override { return row_->angle_delta[index]; }
    PlayerOrientation &orientation() override { return actor_.orientation; }
};

// NOTSA: plain borrowed owners for the shared game calculation.
struct NativeFloor final : FloorContext {
    NativeFloor(Actor &actor, const PlayerMotion &motion, const std::uint16_t &flags,
                const PlayerParameters &parameters, const FloorContact &contact,
                const NinjaVector &gravity, PlayerCoordinates &coordinates, X87Context &fp,
                const MotionConstants &constants)
        : FloorContext(fp, constants), actor_(actor), motion_(motion), flags_(flags),
          parameters_(parameters), contact_(contact), gravity_(gravity), coordinates_(coordinates) {
    }
    Actor &actor_;
    const PlayerMotion &motion_;
    const std::uint16_t &flags_;
    const PlayerParameters &parameters_;
    const FloorContact &contact_;
    const NinjaVector &gravity_;
    PlayerCoordinates &coordinates_;
    const NinjaVector &gravity() override { return gravity_; }
    const NinjaVector &direction() override { return contact_.direction; }
    const NinjaVector &normal() override { return contact_.normal; }
    std::uint16_t flags() override { return flags_; }
    const NinjaVector &speed() override { return motion_.speed; }
    const PlayerParameters &parameters() override { return parameters_; }
    PlayerOrientation &orientation() override { return actor_.orientation; }
    std::int32_t pitch(bool) override { return contact_.pitch; }
    std::int32_t roll(bool) override { return contact_.roll; }
    void world_to_player(NinjaVector &value) override {
        if (!coordinates_.world_to_player(actor_.orientation, value, arithmetic))
            throw MotionError(MotionFailure::CoordinateUnavailable);
    }
};
} // namespace

// NOTSA: native composition of the shared game calculation.
void nudge_scene_normal(Actor &actor, MotionWork &world, const NinjaVector &normal, X87Context &fp,
                        const MotionConstants &constants) {
    NativeNudge context(actor, world, normal, fp, constants);
    nudge_scene_normal(context);
}

// NOTSA: native composition of the shared game calculation.
void move_scene_platform(Actor &actor, const PlayerMotion &motion, NinjaVector &center,
                         const NinjaVector &offset, PlatformWork *work,
                         const std::array<TaskHandle, 8> &players, X87Context &fp,
                         const MotionConstants &constants) {
    NativePlatform context(actor, motion, center, work, players, fp, constants);
    move_scene_platform(context, offset);
}

// NOTSA: native composition of the shared game calculation.
void rotate_scene_floor(Actor &actor, const PlayerMotion &motion, const std::uint16_t &flags,
                        const PlayerParameters &parameters, const FloorContact &contact,
                        const NinjaVector &gravity, PlayerCoordinates &coordinates, X87Context &fp,
                        const MotionConstants &constants) {
    NativeFloor context(actor, motion, flags, parameters, contact, gravity, coordinates, fp,
                        constants);
    rotate_scene_floor(context);
}
} // namespace sadx::scene
