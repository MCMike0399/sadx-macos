#pragma once
#include "game/game_controller.hpp"
#include "game/game_logic_support.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

namespace sadx {
// GameLogicControllers are the live ControllerPointers[0..3] views. The source
// reads/writes pressed at +0x10 of each record, independently of held buttons
// and the analog producer's input tables.
struct GameLogicState {
    GameLogicTimingState& timing;
    std::optional<std::uint32_t> word_at_3abdf70;
    std::optional<std::int32_t> word_at_3b11194;
};
enum class GameLogicOperation { input_edges,calculate_ticks,poll_input,scene_logic,set_mode };
struct GameLogicCall {
    GameLogicOperation operation;
    std::uint32_t source_call,callee;
    std::optional<std::int32_t> first_argument={},second_argument={};
};
enum class GameLogicOwnerState { unavailable,completed,failed,cancelled };
struct GameLogicOwnerDiagnostic { virtual ~GameLogicOwnerDiagnostic()=default; };
struct GameLogicOwnerResult {
    GameLogicOwnerState state=GameLogicOwnerState::unavailable;
    std::uint32_t source_boundary=0;
    std::shared_ptr<const GameLogicOwnerDiagnostic> diagnostic={};
};
struct GameLogicRuntime {
    GameLogicControllers& controllers;
    GameLogicState& state;
    // Required real owners, including40ec80/413920/40f460/413d40/411e30.
    // Callback fixtures can compare orchestration but do not establish that
    // input delivery, scene execution or mode changes are implemented.
    std::function<GameLogicOwnerResult(const GameLogicCall&)> invoke;
    std::function<bool()> cancellation_requested={};
};
enum class GameLogicFailure { none,unknown_controller,null_controller,unknown_pressed,
    unknown_state,missing_owner,owner_failed,cancelled };
struct GameLogicResult {
    GameLogicFailure failure=GameLogicFailure::none;
    std::uint32_t source_boundary=0,nested_source_boundary=0;
    std::uint64_t completed_scene_ticks=0;
    std::optional<GameLogicCall> pending_call;
    std::shared_ptr<const GameLogicOwnerDiagnostic> diagnostic={};
    bool completed=false;
    explicit operator bool()const noexcept{return failure==GameLogicFailure::none&&completed;}
};
// Full local413dd0 control/press aggregation. Runtime owners and all mutable
// state are borrowed; missing values never select a guessed default branch.
GameLogicResult run_game_logic(GameLogicRuntime&);
} // namespace sadx
