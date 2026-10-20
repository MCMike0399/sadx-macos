#pragma once
#include "engine/scene_resource_reference.hpp"
#include "player/player_scene_position_helpers.hpp"
#include <unordered_map>
#include <vector>

namespace sadx::scene {
struct PlayerWork {
    PlayerMotion motion;
    std::uint16_t status;
    std::uint32_t prior_attributes;
    NinjaVector floor_normal, collision_normal;
    std::int16_t no_control_time;
    float alignment;
    TaskHandle moving_task;
    SceneResourceReference cached_object;
};
struct WorldMotion {
    MotionWork motion;
    std::int32_t target_yaw, pitch, roll;
};
struct Contact {
    NinjaVector direction;
    float radius;
    NinjaVector center, displacement;
    std::int32_t pitch, roll;
    std::uint16_t contact_count, selected_contact;
    NinjaVector response_normal, surface_normal, response_plane, corrected_plane, slide_vector;
    NinjaVector previous_normal;
};
struct Globals {
    std::uint32_t special_surface, motion_override;
    TaskHandle event_task;
    std::uint8_t environment;
    std::int8_t render_mode;
    float amy_center_offset, amy_radius_offset;
    NinjaVector gravity;
};
struct LandEntry {
    std::uint32_t attributes;
    SceneResourceReference object, task;
};
struct Landscape {
    std::int32_t candidate_count;
    std::vector<LandEntry> candidates;
};
struct GeometryTask {
    TaskHandle task;
    bool has_primary;
    std::uint16_t flags;
};
struct RegisteredPlayer {
    PlayerWork *player;
    PlayerParameters *parameters;
    WorldMotion *world;
};
enum class Failure {
    NullGeometryPrimary,
    LocalBufferOverflow,
    MissingGeometry,
    CoordinateFailed,
    UncomposedCallee,
    MissingRegisteredPlayer,
    MissingSelectedResponse,
    MissingPlatformWork,
    NullMovingTask,
    UninitializedEntry
};
class Error : public std::runtime_error {
  public:
    Error(Failure failure, const char *operation)
        : std::runtime_error(operation), failure_(failure) {}
    Failure failure() const { return failure_; }

  private:
    Failure failure_;
};
// NOTSA: borrowed native scene owners; missing native callee composition throws Error.
struct Runtime {
    Actor &actor;
    std::uint8_t &character_id, &action;
    PlayerWork &player;
    WorldMotion &world;
    PlayerParameters &parameters;
    Contact &contact;
    Globals &globals;
    Landscape &landscape;
    std::array<RegisteredPlayer, 8> &registered;
    std::unordered_map<std::size_t, GeometryTask> &geometry;
    std::array<TaskHandle, 8> &tasks;
    std::unordered_map<const Task *, PlatformWork *> &platforms;
    const std::vector<NinjaVector> &response_points;
    PlayerCoordinates &coordinates;
    X87Context &arithmetic;
};
std::int32_t position_player_in_scene(Runtime &);
std::int32_t position_player_in_scene_alternate(Runtime &);
} // namespace sadx::scene
