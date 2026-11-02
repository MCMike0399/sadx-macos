#pragma once
#include "engine/collision_component.hpp"
#include "engine/game_controller.hpp"
#include "player/animation/animation.hpp"
#include "player/player_acceleration_air.hpp"
#include "player/player_acceleration_alternate.hpp"
#include "player/player_movement.hpp"
#include "player/player_position.hpp"
#include "player/sonic_player.hpp"
#include "runtime/native_player_system.hpp"
#include "game/editor_state.hpp"
#include <functional>

namespace sadx::sonic {
// NOTSA: shared motion owners borrowed by one controller invocation. Ground physics
// uses the native implementation; these dependencies contain no reference transport.
struct Motion {
    PlayerCoordinates &coordinates;
    const PlayerAccelerationRuntime &acceleration;
    const AirAccelerationConstants &air_constants;
    const AirAccelerationCallees &air_callees;
    const FloorAngleConstants &floor_constants;
    const CrtAsinRuntime &inverse;
    const PlayerPositionConstants &position_constants;
    // NOTSA: the native position composition is still a named missing operation.
    const AlternateSurfaceRuntime *surface = nullptr;
    const CoordinateAxisConstants *axis = nullptr;
    const ProjectionAngleConstants *projection = nullptr;
};
// NOTSA: Root supplies Session's actual owners at the callback. No registry,
// current-player slots, pose, collision scene or player parameters are copied here.
struct Context {
    native::PlayerSystem &players;
    TaskScheduler &scheduler;
    const TaskHandle &task;
    fidelity::Context &arithmetic;
    const std::int16_t *stage = nullptr;
    const std::uint8_t *alternate_model = nullptr;
    const player_initialization::ParameterTable *parameters = nullptr;
    const PlayerEventFlags *events = nullptr;
    TaskCallback auxiliary = nullptr;
    std::shared_ptr<const animation::TableStorage> const *animation_table = nullptr;
    animation::Runtime *animation = nullptr;
    const GameLogicEnginePads *pads = nullptr;
    const GameControllerButtonMasks *button_masks = nullptr;
    const std::array<TaskHandle, 10> *reset_tasks = nullptr;
    TaskCallback spin_effect = nullptr, release_effect = nullptr, release_destructor = nullptr;
    Motion *motion = nullptr;
    // NOTSA: ordered live reads, kept separate even if the first is nonzero.
    // Missing native composition throws rather than assuming an editor default.
    std::function<std::int16_t()> read_editor{}, read_forced{};
    // NOTSA: the Sonic-consumed event link is distinct from preview's action tail.
    // Root binds the canonical event owner after its pinned-PC field identity is settled.
    std::function<std::uint8_t()> read_event_mode{};
    std::function<void(std::uint8_t)> write_event_mode{};
    std::function<bool()> read_event_link{};
};
// NOTSA: native-facing entry points execute the same template body as the reference adapter.
// NOTSA: bind event reads/stores to the callback's captured primary allocation.
void bind_event_work(Context &);
// NOTSA: bind live signed-word reads without copying either source owner.
void bind_editor_state(Context &, editor::View);
void run(Context &);
Reply action_checks(Context &);
Reply action_predicate(Context &, Predicate, std::optional<std::uint32_t> argument = {});
Reply dispatch(Context &);
Reply ground_input(Context &);
Reply copy_local_speed(Context &);
Reply update_collision_center(Context &);
} // namespace sadx::sonic
