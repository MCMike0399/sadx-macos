#pragma once
#include <optional>
#include "engine/game_logic.hpp"
#include <cstdint>
#include <functional>
namespace sadx {
// The message pump 789bd0 (the 7959a0 hook 789c50 registers), from exact
// instructions: PeekMessageA(&msg, [3d0fd30], 0, 0, PM_REMOVE); while a message
// arrives, TranslateMessage + DispatchMessageA (the window procedure is an
// owner) and peek again (rereading 3d0fd30); then 3d08534 = msg.wParam. When the
// first peek finds nothing the MSG buffer was never written, so 3d08534 receives
// an uninitialised stack word: unknown here.
// Profile: the pinned loader's CreateSADXWindow_r WriteJumps 789bd0 to HandleWindowMessages_r
// (window.cpp): PeekMessageA with a null window (every window of the thread), TranslateAccelerator
// (parentWindow, accelTable) first (a handled accelerator skips Translate/Dispatch), no 3d0fd30
// read; the same 3d08534 (its `wParam`) store, uninitialised when nothing was peeked.
enum class GameMessagePumpProfile {original,mod_loader};
struct GameMessage {std::uint32_t hwnd=0,message=0,wparam=0,lparam=0,time=0,x=0,y=0;};
struct GameWindowState {std::optional<std::uint32_t> last_wparam_at_3d08534;};
struct GameMessagePumpRuntime {
    GameWindowState& state;
    const std::optional<std::uint32_t>& window_at_3d0fd30;
    std::function<std::optional<std::optional<GameMessage>>(std::uint32_t hwnd)> peek; // outer nullopt: host unavailable; inner: no message
    std::function<GameLogicOwnerResult(const GameMessage&)> dispatch;                 // TranslateMessage then DispatchMessageA
    GameMessagePumpProfile profile=GameMessagePumpProfile::original;
    std::function<std::optional<bool>(const GameMessage&)> translate_accelerator;     // mod_loader only: true when handled
};
enum class GameWindowFailure {none,unknown_state,host_unavailable,dispatch_failed};
struct GameWindowResult {
    GameWindowFailure failure=GameWindowFailure::none;std::uint32_t source_boundary=0;std::optional<std::uint32_t> required_fact;
    std::uint32_t dispatched=0;
    explicit operator bool()const noexcept{return failure==GameWindowFailure::none;}
};
GameWindowResult run_message_pump_789bd0(GameMessagePumpRuntime&);
} // namespace sadx
