#pragma once
#include <optional>
#include "game/game_logic.hpp"
#include <cstdint>
#include <functional>
namespace sadx {
// The profile's per-frame mod-loader work. The pinned loader (eb4cdcca, DLL
// 3293cf35) WriteJumps 426050's RET at 426063 to ProcessCodes (dllmain.cpp):
//   codeParser.processCodeList()      the profile has no Codes.dat: empty list;
//   RaiseEvents(modFrameEvents)       the Speedrun mod's OnFrame (+0x2e60) is the
//                                     only enabled OnFrame export;
//   uiscale::check_stack_balance()    prints only (debug outputs are off);
//   the debug message queue           empty (DebugScreen is off).
// ProcessCodes then returns to 426050's caller 40c9b6, which does not read EAX.
// Speedrun 2.0 (f3fc68a3) globals this owner holds. Init leaves them as:
// 100107e9 = QuickSaveSettings.Enabled == "true" (_stricmp), 10001080 clears
// 100107e8 (set only by its 71adce Chao-race hook), 100012a0 clears the timer.
struct SpeedrunModState {
    std::optional<std::uint8_t> chao_viewer_at_100107e8,quick_save_at_100107e9;
    std::optional<std::uint8_t> timer_frames_at_10010818,timer_seconds_at_10010819,timer_minutes_at_1001081a;
};
SpeedrunModState speedrun_mod_after_init(bool quick_save_enabled);
struct SpeedrunFrameRuntime {
    SpeedrunModState& mod;
    const std::optional<std::uint16_t>& game_mode_at_3b22de4;  // GameLoopState
    const std::optional<std::int32_t>& mode_at_3abdc7c;         // the runtime's 40c090 mode owner
    const std::optional<std::uint16_t>& stage_at_3b22dcc;       // SonicPlayerGlobals
    // Nested owners: 10004f80 (quick save) and the Chao stat viewer body 10002e7f..10002f4e.
    std::function<GameLogicOwnerResult(std::uint32_t source_call,std::uint32_t callee)> invoke;
};
enum class ModFrameFailure {none,unknown_state,missing_owner,owner_failed,cancelled};
struct ModFrameResult {
    ModFrameFailure failure=ModFrameFailure::none;
    std::uint32_t source_boundary=0; // DLL VA
    std::optional<std::uint32_t> required_fact,pending_callee;
    explicit operator bool()const noexcept{return failure==ModFrameFailure::none;}
};
// Speedrun 2.0 OnFrame (+0x2e60) from its exact instructions.
ModFrameResult run_speedrun_on_frame(SpeedrunFrameRuntime&);
// ProcessCodes under the profile: the Speedrun OnFrame is its only effect.
ModFrameResult run_mod_loader_process_codes(SpeedrunFrameRuntime&);
} // namespace sadx
