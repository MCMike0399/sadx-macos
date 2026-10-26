#pragma once
#include "player/actor_current.hpp"
#include "engine/game_controller.hpp"
#include "player/player_startup.hpp"
#include "player/player_position.hpp"
#include "player/player_movement.hpp"
#include "player/player_acceleration_air.hpp"
#include "player/player_acceleration_alternate.hpp"
#include "player/player_animation.hpp"
#include <functional>
namespace sadx {
enum class SonicPlayerFailure {
    none,unknown_field,null_required_resource,unresolved_resource,
    missing_native_owner,native_owner_failed,unimplemented_action,
    coordinate_failed,input_failed,acceleration_failed,speed_failed,position_failed,
    startup_failed,unknown_motion_binding,unknown_player_binding,work_binding_failed,conflicting_live_binding,unknown_control,animation_failed
};
struct SonicPlayerResult {
    SonicPlayerFailure failure=SonicPlayerFailure::none;
    std::uint32_t source_boundary=0,callee_address=0,nested_source_boundary=0;
    bool completed=false;
    std::optional<std::uint32_t> value;
    std::optional<std::uint8_t> dispatched_action;
    std::optional<std::uint32_t> dispatch_target;
    std::optional<PlayerAccelerationResult> acceleration;
    std::optional<AirAccelerationResult> air;
    std::optional<ProjectionAngleResult> projection;
    std::optional<PlayerMovementResult> movement;
    std::optional<PlayerPositionResult> position;
    std::optional<CoordinateResult> coordinate;
    std::optional<FloorAnglesResult> floor_angles;
    std::optional<PlayerInputResult> input;
    std::optional<PlayerStartupResult> startup;
    std::optional<TaskWorkFailure> work_failure;
    std::optional<PositionHoldResult> hold;
    std::optional<PositionNormalResult> normal;
    std::optional<PlayerAnimationResult> animation;
    std::optional<TaskResult> task_creation;
    std::optional<std::shared_ptr<const PlayerTaskState>> task_work_return;
    explicit operator bool()const noexcept{return failure==SonicPlayerFailure::none&&completed;}
};
struct SonicPlayerFrame {
    TaskHandle object;
    std::shared_ptr<PlayerTaskState> task;
    std::shared_ptr<PlayerMotionState> world;
    // Unknown native view differs from a known-null motionwk+0 reference.
    std::optional<std::shared_ptr<PlayerStartupPlayer>> player;
    PlayerPositionCollisionScene* collision;
};
struct SonicPlayerFrameBinding {
    SonicPlayerResult result;
    std::optional<SonicPlayerFrame> frame;
    explicit operator bool()const noexcept{return static_cast<bool>(result)&&frame.has_value();}
};
// Resolve actual motionwk+0 through the arena reference owner and registry.
// The live world view is explicit: no motion numeric state is synthesized.
SonicPlayerFrameBinding bind_sonic_player_frame(const TaskHandle&,const TaskWorkArena&,
    std::shared_ptr<PlayerMotionState>,const PlayerStartupRegistry&,PlayerPositionCollisionScene*);
struct SonicPlayerGlobals {
    std::optional<std::int16_t> forced_at_3b29d0c;
    const std::optional<std::uint16_t>* forced_at_3b29d08=nullptr; // borrowed GameResetState word; null is unknown
    std::optional<std::uint16_t> stage_at_3b22dcc;
    std::optional<std::uint8_t> alternate_model_at_3b18db5;
    std::optional<std::uint8_t> sound_latch_at_3c55d44;
    std::array<std::optional<TaskHandle>,10> tasks_at_3c5a27c;
    actor_current::State current{};
    const actor_current::Observation* current_observation = nullptr;
};
struct SonicPlayerConstants {
    static constexpr std::array<std::uint32_t,13> addresses{{
        0x7dcb2c,0x7dfff0,0x7dd554,0x7e0090,0x7dd57c,0x7df1bc,0x7e01a8,0x7dcb94,0x7e0844,0x7df1c8,0x7e00f4,0x7dcb34,0x7dd550}};
    std::array<std::uint32_t,13> bits;
    float at(std::uint32_t)const;
};
std::optional<SonicPlayerConstants> decode_sonic_player_constants(const std::uint8_t*,std::size_t);
// Source-call contract for genuinely unported game/animation/resource owners.
// A missing owner or incomplete result is a boundary, never implicit success.
struct SonicPlayerCall {
    std::uint32_t address,call_site;
    std::array<std::optional<std::uint32_t>,5> integer_arguments;
    // A pointer argument is this frame's authoritative taskwk, not an integer.
    std::optional<std::uint8_t> task_work_argument_index={};
};
using SonicPlayerExternalOwner=std::function<SonicPlayerResult(
    const SonicPlayerCall&,SonicPlayerFrame&,X87Context&)>;
// Borrowed live engine pads (source3b0e7f0, held+8/pressed+0x10) and the one
// mutable source button-mask owner written by40fdc0. Null means the owner is
// unavailable. These are not OS physical button labels.
struct SonicPlayerControls {
    const GameLogicEnginePads* engine_pads=nullptr;
    const GameControllerButtonMasks* masks=nullptr;
};
enum class SonicPlayerPredicateEntry : std::uint32_t {
    event_action_495fa0=0x495fa0,roll_release_492f90=0x492f90,fall_494f70=0x494f70,
    jump_495e60=0x495e60,hold_493120=0x493120,charge_496ee0=0x496ee0,
    stage_pickup_442680=0x442680,stage_action_442570=0x442570,
    jump_press_43bf40=0x43bf40,jump_classify_4b83f0=0x4b83f0,
    clear_tasks_4b8320=0x4b8320,idle_494ff0=0x494ff0,
    turn_break_4429c0=0x4429c0,request_spin_animation_495c30=0x495c30,request_jump_animation_495b50=0x495b50,
    request_walk_animation_495ba0=0x495ba0,wall_form_43bfe0=0x43bfe0,skid_end_4930d0=0x4930d0,charge_tick_495080=0x495080,
};
struct SonicGroundInputRuntime {
    const AlternateSurfaceRuntime& surface;
    const CoordinateAxisConstants& axis;
    const ProjectionAngleConstants& projection;
    double angle_units; // source7dccd0
    double positive_factor; // source7dccc8
};
struct SonicPlayerNativeMotion {
    PlayerCoordinates& coordinates;
    const PlayerAccelerationRuntime& acceleration_runtime;
    const GroundAccelerationConstants& ground_constants;
    const GroundAccelerationCallees& ground_callees;
    const AirAccelerationConstants& air_constants;
    const AirAccelerationCallees& air_callees;
    const FloorAngleConstants& floor_constants;
    const CrtAsinRuntime& inverse_math;
    const PlayerPositionConstants& position_constants;
    const PlayerRotationConstants& rotation_constants;
    const PlayerPositionRuntime& position_runtime;
    const PlayerPositionCallees& position_callees;
    const SonicGroundInputRuntime* ground_input_projection=nullptr;
};
struct SonicPlayerRuntime {
    SonicPlayerGlobals& globals;
    TaskScheduler& scheduler;
    PlayerStartupRegistry& players;
    const PlayerStartupRuntime* startup_runtime;
    const SonicPlayerNativeMotion* motion;
    const SonicPlayerConstants& constants;
    SonicPlayerExternalOwner external;
    const SonicPlayerControls* controls=nullptr;
    const std::unordered_map<std::size_t,TaskHandle>* collision_tasks=nullptr;
    TaskCallback spin_effect_callback_495a50=nullptr;
    TaskCallback spin_release_effect_callback_4940e0=nullptr;
    TaskCallback spin_release_destructor_494120=nullptr;
    PlayerAnimationRuntime* animation_runtime=nullptr;
    std::optional<std::shared_ptr<const PlayerAnimationTable>> initialized_animation_table={};
};

// Actual dispatch table, not Hopper's regrouped pseudocode case indices.
std::optional<std::uint32_t> sonic_action_target(std::uint8_t);
SonicPlayerResult sonic_ground_input(SonicPlayerFrame&,const SonicPlayerNativeMotion&,
    const SonicPlayerConstants&,X87Context&);
// Local43ee70 and4915f0 owners are exposed for direct original-code goldens.
SonicPlayerResult sonic_copy_local_speed(SonicPlayerFrame&,const SonicPlayerNativeMotion&,
    const SonicPlayerConstants&,X87Context&);
// Actual post-transform suffix43eecc, with explicitly supplied source stack vector.
// Exposed for separately labeled seeded-suffix instruction comparisons.
SonicPlayerResult sonic_finalize_local_speed(SonicPlayerFrame&,NinjaVector&,
    const SonicPlayerConstants&,const std::optional<NinjaVector>& world_gravity,X87Context&);
SonicPlayerResult sonic_update_collision_center(SonicPlayerFrame&,PlayerCoordinates&,X87Context&);
// Actual496f50 prefix and closed null/init guards. Remaining predicates stop
// at their mandatory source call, preserving earlier task timer/flag stores.
SonicPlayerResult sonic_action_checks(SonicPlayerFrame&,SonicPlayerRuntime&,X87Context&);
// Full source entry composition for recovered branches; every unresolved
// animation/task/model/scene owner remains explicit. Does not claim playability.
SonicPlayerResult run_sonic_player(SonicPlayerFrame&,SonicPlayerRuntime&,X87Context&);
// Source leaf predicate seam; scalar arguments are only consumed by actual
// procedures that accept them (e.g. packed stage action442570).
SonicPlayerResult sonic_action_predicate(SonicPlayerPredicateEntry,SonicPlayerFrame&,
    SonicPlayerRuntime&,X87Context&,std::optional<std::uint32_t> scalar_argument={});
// Actual jump-table seam permits isolated state-body validation. It does not
// replace or skip496f50 in run_sonic_player.
SonicPlayerResult dispatch_sonic_player_action(SonicPlayerFrame&,SonicPlayerRuntime&,X87Context&);
} // namespace sadx
