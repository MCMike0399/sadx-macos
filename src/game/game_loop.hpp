#pragma once
#include "engine/game_frame_control.hpp"
#include "engine/game_logic.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

namespace sadx {
// Globals read/written by the local body of GameLoop 0x40cad0. Callees own the
// rest of the game state; their effects arrive through the borrowed owners.
struct GameLoopState {
    std::optional<std::uint32_t> word_at_3abdcf8,exit_at_3abdf7c,focus_at_8929d4;
    std::optional<std::uint8_t> resync_at_3abdf68;
    // Read-only here; borrowed from their owners (queue consumer state 909eb4,
    // sprite queue runtime 3abdcfe). Null is an unknown fact at the read.
    const std::optional<std::uint8_t>* special_flags_at_909eb4=nullptr;
    const std::optional<std::uint8_t>* fog_flag_at_3abdcfe=nullptr;
    std::optional<std::uint32_t> fog_range_at_3abdca0,fog_range_at_3abdca4,fog_range_at_3abdc70,fog_range_at_3abdc74;
    std::optional<std::uint8_t> request_at_3b291ad,counter_at_3b291b2,pending_at_3abdf76,pending_at_3abdf79;
    std::optional<std::int32_t> save_phase_at_3b291a4;
    std::optional<std::uint32_t> word_at_3abdf80,mode_at_3b28114,frames_at_3abdf58,frames_at_3abdf5c,word_at_3c85804;
    std::optional<std::uint16_t> game_mode_at_3b22de4;
};
// Every callee in source order. 51f680 is a verified RET; it is still issued
// so callers keep one ordered owner trace, but it has no required effect.
enum class GameLoopOperation : std::uint32_t {
    init_40c030,ret_51f680,set_401410,message_77f090,input_40f460,idle_781010,idle_781060,
    work_420830,queue_reset_407ab0,depth_77ed00,work_40c090,mode_4250d0,fog_411b60,fog_range_403180,
    consume_4086f0,save_40be40,pending_40e2c0,work_4326f0,work_40d4d0,work_40eab0,work_42f990,work_40d3c0,
    work_420840,frame_411d00,work_421fd0,keys_40ef30,key_40ef20,shutdown_7853c0,shutdown_6fbf10,
    shutdown_410630,shutdown_420f40,shutdown_420ef0,shutdown_77fc50 };
struct GameLoopCall {
    GameLoopOperation operation;
    std::uint32_t source_call,callee;
    std::array<std::optional<std::uint32_t>,2> arguments={};
};
struct GameLoopOwnerResult {
    GameLogicOwnerResult owner;
    std::optional<std::uint32_t> return_value; // EAX for 40ef30/40ef20 only
};
struct GameLoopRuntime {
    GameLoopState& state;
    GameLogicState& logic; // sole owner of 3b11180/88/94/98 (via logic.timing)
    // Required real owners. A fixture can compare ordering but does not
    // establish that the owning subsystem is implemented.
    std::function<GameLoopOwnerResult(const GameLoopCall&)> invoke;
    std::function<bool()> cancellation_requested={};
};
enum class GameLoopFailure {none,unknown_state,missing_owner,owner_failed,missing_return,cancelled};
enum class GameLoopNext {iteration,shutdown,finished};
struct GameLoopResult {
    GameLoopFailure failure=GameLoopFailure::none;
    std::uint32_t source_boundary=0,nested_source_boundary=0;
    std::optional<GameLoopCall> pending_call;
    std::shared_ptr<const GameLogicOwnerDiagnostic> diagnostic={};
    GameLoopNext next=GameLoopNext::iteration;
    bool completed=false;
    explicit operator bool()const noexcept{return failure==GameLoopFailure::none&&completed;}
};
// 411d00 extra-tick policy at 411d1c. The original lag-recovery span
// (411d1c..411da1, 7800e0/780120/780130 timing) is not ported and stays a
// required boundary; Speedrun 2.0 replaces it with max(0, FrameMultiplier-1)
// read from 3b11188 and resumes at 411da3 (DLL +0x11c0/+0x11d0, pinned f3fc68a3).
enum class FrameCompletionPolicy {original_unported,speedrun_2_0};
enum class FrameCompletionOperation : std::uint32_t {
    thunk_781010,thunk_781020,thunk_781030,thunk_781040,thunk_781050,limiter_781060 };
struct FrameCompletionCall {FrameCompletionOperation operation;std::uint32_t source_call,callee;};
struct FrameCompletionRuntime {
    GameLoopState& loop;            // 3abdf68, 3b28114
    GameLogicState& logic;          // 3b11194 and timing 3b11180/88/98
    GameFrameRendererState& renderer; // render factor 89295c
    FrameCompletionPolicy policy=FrameCompletionPolicy::speedrun_2_0;
    std::function<GameLogicOwnerResult(const FrameCompletionCall&)> invoke;
};
enum class FrameCompletionFailure {none,unknown_state,unported_original_policy,missing_owner,owner_failed,cancelled};
struct FrameCompletionResult {
    FrameCompletionFailure failure=FrameCompletionFailure::none;
    std::uint32_t source_boundary=0,nested_source_boundary=0;
    std::optional<FrameCompletionCall> pending_call;
    std::optional<std::int32_t> extra_ticks; // value stored to 3b11180
    bool completed=false;
    explicit operator bool()const noexcept{return failure==FrameCompletionFailure::none&&completed;}
};
FrameCompletionResult run_frame_completion(FrameCompletionRuntime&); //411d00
// 40cad0..40caf9: initialization, then the source exit test.
GameLoopResult run_game_loop_startup(GameLoopRuntime&);
// 40cb00..40cca7: one source frame, ending with the 40ef30/40ef20/exit test.
GameLoopResult run_game_loop_iteration(GameLoopRuntime&);
// 40ccaf..: shutdown owners ending in the 77fc50 tail jump.
GameLoopResult run_game_loop_shutdown(GameLoopRuntime&);
} // namespace sadx
