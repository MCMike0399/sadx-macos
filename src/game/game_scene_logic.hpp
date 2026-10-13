#pragma once
#include "engine/game_logic.hpp"
#include "engine/task.hpp"
#include "cpu/x87_math.hpp"

namespace sadx {
struct GameSceneLogicState {
    // Borrow the stage word already owned by Sonic/world globals. Signed
    // comparisons use its original bits; no second stage value is initialized.
    std::optional<std::uint16_t>& stage_at_3b22dcc;
    const std::optional<std::int32_t>* mode_at_3abdc7c=nullptr; // borrowed game mode owner; null is unknown
    std::optional<std::uint8_t> byte_at_3b0ef40,byte_at_3b0f150;
};
enum class GameSceneOperation { input_40fdc0,context_4014b0,conditional_5919c0,
    collision_420700,collision_456ec0,registry_4511d0,query_42fb00,conditional_426070 };
struct GameSceneCall {GameSceneOperation operation;std::uint32_t source_call,callee;};
struct GameSceneOwnerResult {
    GameLogicOwnerResult owner;
    std::optional<std::int32_t> return_value;
};
struct GameSceneLogicRuntime {
    GameSceneLogicState& state;
    TaskScheduler& scheduler;
    X87Context& arithmetic;
    std::function<GameSceneOwnerResult(const GameSceneCall&,X87Context&)> invoke;
};
enum class GameSceneLogicFailure {none,unknown_mode,unknown_stage,missing_owner,
    owner_failed,missing_return,task_failed,cancelled};
struct GameSceneLogicResult {
    GameSceneLogicFailure failure=GameSceneLogicFailure::none;
    std::uint32_t source_boundary=0,nested_source_boundary=0;
    std::optional<GameSceneCall> pending_call;
    std::optional<TaskResult> task_failure;
    std::shared_ptr<const GameLogicOwnerDiagnostic> diagnostic={};
    bool tasks_completed=false,completed=false;
    explicit operator bool()const noexcept{return failure==GameSceneLogicFailure::none&&completed;}
};
// Full local413d40 ordering, using the actual native task scheduler. Called
// input/collision/registry owners are required; no missing owner becomes a
// successful step. The caller's one arithmetic context reaches every owner.
GameSceneLogicResult run_game_scene_logic(GameSceneLogicRuntime&);
} // namespace sadx
