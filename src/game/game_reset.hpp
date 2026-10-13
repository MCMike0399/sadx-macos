#pragma once
#include "cpu/crt_rand.hpp"
#include "game/game_logic.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>

namespace sadx {
// 40bf40 game-state reset. Stores: 3abdf80=0, word 3b29d08=0, and the two
// heap pointers 3b290d0 / 3b0e330 are freed (780fb0) and zeroed when nonzero.
// It ends by reseeding the CRT RNG with 0xdead0cab (srand 6443b5).
struct GameResetState {
    std::optional<std::uint32_t>& word_at_3abdf80; // borrowed from GameLoopState
    std::optional<std::uint16_t> word_at_3b29d08;
    std::optional<std::uint32_t> heap_at_3b290d0,heap_at_3b0e330;
    CrtRandState& rng;
};
struct GameResetCall {std::uint32_t source_call,callee;std::optional<std::uint32_t> argument;};
struct GameResetRuntime {
    GameResetState& state;
    // Required owners for every callee except the verified 51f680 RET and srand.
    std::function<GameLogicOwnerResult(const GameResetCall&)> invoke;
};
enum class GameResetFailure {none,unknown_state,missing_owner,owner_failed,cancelled};
struct GameResetResult {
    GameResetFailure failure=GameResetFailure::none;
    std::uint32_t source_boundary=0,nested_source_boundary=0;
    std::optional<GameResetCall> pending_call;
    bool completed=false;
    explicit operator bool()const noexcept{return failure==GameResetFailure::none&&completed;}
};
GameResetResult run_game_reset_40bf40(GameResetRuntime&);
} // namespace sadx
