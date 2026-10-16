#pragma once
#include "game/game_loop.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

namespace sadx {
// Globals owned by the 40c090 game-mode machine that no other native owner
// holds. Shared words (3b28114, 3b291a4, 3b291ad, 3abdcf8) are borrowed from
// GameLoopState; the game mode 3abdc7c is borrowed from its existing owner.
struct GameModeState {
    std::optional<std::uint8_t> byte_at_3abdf74,byte_at_3abdcfc,byte_at_3abdcfd,byte_at_3b18db4,byte_at_3b2a2fa,byte_at_3b22e1e,byte_at_3abdf78;
    std::optional<std::uint32_t> word_at_3b2c45c,word_at_3b290e4;
    // *(short*)(*(void**)0x3b0ef4c + 4), reloaded at every source use. The
    // pointer and its target are one fact here: unknown or null both stop at
    // the MOVSX dereference (where a null pointer faults in the source).
    std::optional<std::int16_t> level_word_via_3b0ef4c;
};
// One callee invocation in source order. Arguments are the cdecl stack words.
struct GameModeCall {
    std::uint32_t source_call,callee;
    std::array<std::optional<std::uint32_t>,3> arguments={};
};
struct GameModeOwnerResult {
    GameLogicOwnerResult owner;
    std::optional<std::uint32_t> return_value; // EAX, required only where the source consumes it
};
struct GameModeRuntime {
    GameLoopState& loop;
    std::optional<std::int32_t>& mode_at_3abdc7c;
    GameModeState& state;
    // Required real owners (menus, saves, file loads, cutscenes, audio, ...).
    std::function<GameModeOwnerResult(const GameModeCall&)> invoke;
};
enum class GameModeFailure {none,unknown_state,missing_owner,owner_failed,missing_return,cancelled};
struct GameModeResult {
    GameModeFailure failure=GameModeFailure::none;
    std::uint32_t source_boundary=0,nested_source_boundary=0;
    std::optional<GameModeCall> pending_call;
    bool completed=false;
    explicit operator bool()const noexcept{return failure==GameModeFailure::none&&completed;}
};
// Full local 40c090 (called once per frame from 40cb3b). It returns 0 in EAX;
// its stack local at [esp+0xc] is written but never read and is not modeled.
GameModeResult run_game_mode_40c090(GameModeRuntime&);
} // namespace sadx
