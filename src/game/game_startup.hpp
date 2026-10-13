#pragma once
#include "game/game_controller.hpp"
#include "game/game_frame_control.hpp"
#include "game/game_logic.hpp"
#include "game/game_logic_support.hpp"
#include "input/input_mod_pad.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

namespace sadx {
// 40f350: clear the per-key edge rows (3b0e3e0..3b0e6df), 3b0e3b8..3ba and the
// five controller words 3b0e990..9a0. Pure stores.
void clear_input_edges_40f350(GameLogicEdgeState&,GameFrameInputState&,GameControllerInitGlobals&);

// 789930 (via thunk 781000): store a callback in the first null slot of the
// 32-slot list at 3d084b0; returns 1, or 0 when full. Slots hold the source
// identity of the registered function (an owner name, not a native pointer).
struct ExitCallbackList {std::array<std::optional<std::uint32_t>,32> slots_at_3d084b0;}; // 0 = empty
struct ExitCallbackResult {std::optional<std::uint32_t> returned;std::uint32_t source_boundary=0;};
ExitCallbackResult register_exit_callback_789930(std::uint32_t source_function,ExitCallbackList&);

// 40a8a0: if byte 900e88 is 0, copy the 48-byte block 900e88 -> 9008b4 and
// rewrite its first four bytes to 00 00 00 01; always 3abdbc1=1, 900e88=0, 903e58=ff.
struct MenuBlockState {
    std::array<std::optional<std::uint8_t>,48> block_at_900e88,block_at_9008b4;
    std::optional<std::uint8_t> byte_at_3abdbc1,byte_at_903e58;
};
bool reset_menu_block_40a8a0(MenuBlockState&); // false: unknown 900e88 before any store

// 424c20 local stores around its 410510 owner call.
struct AudioStartupState {
    std::optional<std::uint32_t> word_at_3b29ce0,word_at_3b29ce4,word_at_3b29b7c;
    std::array<std::optional<std::uint32_t>,36> record_words_at_3b292fc; // stride 0x38
    std::optional<std::uint8_t> byte_at_3b29ce8;
    std::array<std::optional<std::uint32_t>,18> words_at_3b292a8;
};

enum class StartupOperation : std::uint32_t {queue_408630,ret_78c5a0,set_mode_411e30,audio_410510,reset_40bf40,audio_objects_40d0d0};
struct StartupCall {StartupOperation operation;std::uint32_t source_call,callee;std::array<std::optional<std::uint32_t>,2> arguments={};};
struct StartupOwners {
    GameLogicEdgeState& edges;GameFrameInputState& input;GameControllerInitGlobals& controller_globals;
    const GameLogicController& controller_template_at_3b0eaa8;
    const std::array<std::shared_ptr<GameLogicController>,4>& controllers_raw_at_3b0e9c8;
    GameLogicControllers& controller_pointers_at_3b0e77c;
    // Updated profile: the Input mod's 40f451 RET hook repoints the pointers.
    const InputModRawRecords* input_mod_raw_input_at_1000e748;
    AudioStartupState& audio;MenuBlockState& menu;ExitCallbackList& exit_callbacks;
    std::optional<std::int32_t>& mode_at_3abdc7c;
    std::optional<std::uint32_t> &word_at_3abdc80,&word_at_3abdc78,&word_at_3abdc90;
};
struct StartupRuntime {
    StartupOwners owners;
    std::function<GameLogicOwnerResult(const StartupCall&)> invoke; // required owners
};
enum class StartupFailure {none,unknown_state,missing_owner,owner_failed,cancelled,controller_record_missing};
struct StartupResult {
    StartupFailure failure=StartupFailure::none;
    std::uint32_t source_boundary=0,nested_source_boundary=0;
    std::optional<StartupCall> pending_call;
    bool exit_callback_registered=false,completed=false;
    explicit operator bool()const noexcept{return failure==StartupFailure::none&&completed;}
};
// InitGameLoop 40c030 in source order.
StartupResult run_startup_40c030(StartupRuntime&);
} // namespace sadx
