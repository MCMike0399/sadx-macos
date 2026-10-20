#pragma once
#include "player/player_contact_attributes.hpp"
#include "player/player_contact_initializer.hpp"
#include "player/player_contact_response.hpp"
#include "player/player_mesh_registry.hpp"
#include "player/player_scene_position.hpp"
#include "player/player_startup.hpp"

namespace sadx {
enum class ScenePositionFailure {
    none,
    unavailable_field,
    unavailable_contact_work,
    null_motion,
    unavailable_geometry_task,
    null_geometry_primary,
    unavailable_live_primary,
    unresolved_resource,
    coordinate_failed,
    initializer_failed,
    broadphase_failed,
    mesh_query_failed,
    response_failed,
    attributes_failed,
    projection_failed,
    floor_angles_failed,
    asin_failed,
    unavailable_registry_owner,
    unavailable_platform_work,
    unavailable_platform_row,
    unavailable_platform_callback,
    platform_callback_failed,
    unavailable_render_state,
    render_owner_unavailable,
    source_local_buffer_overflow,
    null_moving_task,
};
struct PlayerScenePositionResult {
    ScenePositionFailure failure = ScenePositionFailure::none;
    std::uint32_t source_boundary = 0, nested_source_boundary = 0;
    std::optional<std::int32_t> outcome;
    bool completed = false; // void helper completion, distinct from an integer return
    std::int32_t provisional_outcome = 0;
    std::optional<std::uint16_t> previous_flags;
    std::optional<NinjaVector> center_offset, half_height_offset, temporary_vector;
    std::optional<SceneQueryResult> broadphase;
    std::optional<PlayerMeshQueryResult> mesh_query;
    std::optional<PlayerContactResponseResult> response;
    std::optional<PlayerContactInitializationResult> initialization;
    std::optional<ContactAttributeResult> attributes;
    std::optional<ProjectionAngleResult> projection;
    std::optional<FloorAnglesResult> floor_angles;
    std::optional<CrtAsinResult> asin;
    std::optional<PlayerInputResult> input;
    std::shared_ptr<const PlayerScenePositionResult> platform_callback;
    CoordinateResult coordinate;
    std::size_t candidate_index = 0, processed_count = 0, response_index = 0;
    std::optional<std::size_t> winning_index;
    std::array<std::optional<SceneLandEntry>, 64> first_pass;
    std::array<std::optional<SceneLandEntry>, 64> ordered;
    explicit operator bool() const noexcept {
        return failure == ScenePositionFailure::none && (completed || outcome.has_value());
    }
};
struct ScenePlatformRow;
using ScenePlatformCallback = PlayerScenePositionResult (*)(const TaskHandle &player_task,
                                                            PlayerTaskState &live_actor,
                                                            ScenePlatformRow &, X87Context &);
struct ScenePlatformRow {
    std::optional<ScenePlatformCallback> callback; // unknown vs observed null
    std::optional<std::array<std::int32_t, 3>> angle_delta;
    std::optional<NinjaVector> displacement;
};
struct ScenePlatformWork {
    std::array<std::optional<ScenePlatformRow>, 2> rows;
};
struct ScenePlatformRuntime {
    const PlayerStartupRegistry *players;
    std::unordered_map<const Task *, ScenePlatformWork *> *platform_work;
};
struct PlayerScenePositionGlobals {
    std::optional<std::uint32_t> special_surface_at_3b42e74, motion_override_at_3b42e70;
    std::optional<TaskHandle> event_task_at_3b2c578;
    std::optional<std::uint8_t> environment_at_3b22dec;
    std::optional<std::int8_t> render_at_3b0f0ec;
    std::optional<float> amy_offset_at_915d74, amy_radius_at_915d78;
};
struct PlayerScenePositionConstants {
    static constexpr std::array<std::uint32_t, 25> float_addresses{
        {0x7dcb10, 0x7dcb2c, 0x7dcc74, 0x7dcc98, 0x7dccd8, 0x7dd540, 0x7dd550, 0x7dd570, 0x7dd578,
         0x7dd584, 0x7dd58c, 0x7dfff0, 0x7e0084, 0x7e0088, 0x7e008c, 0x7e00e4, 0x7e0118, 0x7e0128,
         0x7e012c, 0x7e0174, 0x7e0178, 0x7e017c, 0x7e0180, 0x7e0184, 0x7e0188}};
    static constexpr std::array<std::uint32_t, 3> double_addresses{{0x7dccc8, 0x7dccd0, 0x7dcce8}};
    std::array<std::uint32_t, 25> float_bits;
    std::array<std::uint64_t, 3> double_bits;
    float single(std::uint32_t) const;
    double wide(std::uint32_t) const;
};
std::optional<PlayerScenePositionConstants> decode_scene_position_constants(const std::uint8_t *,
                                                                            std::size_t);
struct PlayerScenePositionRuntime {
    PlayerScenePositionGlobals &globals;
    PlayerMeshQueryState &contacts;
    PlayerMeshQueryRuntime &query_math;
    const PlayerContactResponseRuntime &response_math;
    const PlayerInputRuntime &input;
    const ForcedInputAngleConversion &forced_angle;
    ScenePlatformRuntime platform;
    // Borrow Task/primary-state owners through normalized resource IDs.
    const std::unordered_map<std::size_t, TaskHandle> *geometry_tasks;
    // Task.force_work identity remains on Task. This is its typed native view;
    // no arena bytes are cast into an address or callback.
    const ContactParameterTable &parameter_defaults;
    const PlayerContactAttributeConstants &attribute_constants;
    const PlayerContactResponseConstants &response_constants;
    const PlayerContactInitializationConstants &initializer_constants;
    const PlayerRotationConstants &rotation_constants;
    const FloorAngleConstants &floor_constants;
    const PlayerMeshQueryConstants &query_constants;
    const SceneBroadphaseConstants &broadphase_constants;
    const CoordinateAxisConstants &axis_constants;
};
// Actual449500/445d10. These routines consume an already refreshed native
// landscape, exactly as the source callers do; no hidden registration step.
PlayerScenePositionResult position_player_in_scene(PlayerTaskState &, PlayerMotionState *,
                                                   PlayerPhysicsState &, PlayerParameters &,
                                                   PlayerSceneLandscape &,
                                                   const PlayerScenePositionConstants &,
                                                   PlayerScenePositionRuntime &,
                                                   PlayerCoordinates &, X87Context &);
PlayerScenePositionResult
position_player_in_scene_alternate(PlayerTaskState &, PlayerMotionState *, PlayerPhysicsState &,
                                   PlayerParameters &, PlayerSceneLandscape &,
                                   const PlayerScenePositionConstants &,
                                   PlayerScenePositionRuntime &, PlayerCoordinates &, X87Context &);
// Actual leaf helpers, exposed to isolate their custom source ABI in probes.
PlayerScenePositionResult move_scene_platform(PlayerTaskState &, PlayerPhysicsState &,
                                              PlayerContactWork &, const NinjaVector &offset,
                                              const ScenePlatformRuntime &,
                                              const CoordinateAxisConstants &,
                                              X87Context &); // 43ca40
PlayerScenePositionResult nudge_scene_normal(PlayerTaskState &, PlayerMotionState *,
                                             const PlayerContactWork &,
                                             const PlayerScenePositionConstants &,
                                             X87Context &); // 43cb40
PlayerScenePositionResult rotate_scene_floor(PlayerTaskState &, PlayerPhysicsState &,
                                             const PlayerParameters &, const PlayerContactWork &,
                                             const PlayerScenePositionConstants &,
                                             const std::optional<NinjaVector> &world_gravity,
                                             PlayerCoordinates &, X87Context &); // 444f10
} // namespace sadx
