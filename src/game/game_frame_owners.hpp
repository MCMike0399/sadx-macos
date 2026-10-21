#pragma once
#include "engine/game_logic_support.hpp"
#include "game/game_view.hpp"
#include "game/menu_caption.hpp"
#include "engine/ninja_queue_consumer.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
namespace sadx {
// Per-frame GameLoop owners after 4086f0 (40d4d0, 40eab0, 42f990, 40d3c0,
// 420840) and the key queries 40ef30/40ef20, each from exact instructions.
// Draw lists, the 40eab0 effect and texture-release walks are nested owners.
struct GameFrameOwnersState {
    std::optional<std::uint32_t> list_at_3b2c584,count_at_3b0e334;
    menu_caption::State caption;
};
struct GameFrameOwnerCall {std::uint32_t source_call,callee;std::array<std::optional<std::uint32_t>,2> arguments={};};
enum class GameFrameOwnerFailure {none,not_ported,unknown_state,owner_failed,missing_owner,render_failed,fp_failed};
struct GameFrameOwnerResult {
    GameFrameOwnerFailure failure=GameFrameOwnerFailure::none;std::uint32_t source_boundary=0;std::optional<std::uint32_t> required_fact,eax;
    std::optional<GameFrameOwnerCall> pending_call;
    explicit operator bool()const noexcept{return failure==GameFrameOwnerFailure::none;}
};
using GameFrameOwnerInvoke=std::function<GameLogicOwnerResult(const GameFrameOwnerCall&)>;
struct GameFrameOwnersRuntime {
    GameFrameOwnersState& state;
    GameViewState& view;                                      // 3b2cbb4, 3ab98ec, 3aad0bc
    std::optional<std::uint8_t>& view_reset_at_3b280a8;
    std::optional<std::uint32_t>& draw_count_at_3b0e338;      // ResetLeafState
    const std::optional<std::uint32_t>& effect_at_3b0e998;    // controller globals
    const std::optional<std::uint8_t>& keys_at_3b0e3b8;       // GameFrameInputState
    const GameLogicEdgeState& edges;                          // 3b0e3e0 + 3k
    NinjaRenderState& render;const std::array<std::uint32_t,8>* depth_table;
    const std::optional<NinjaMath::SineTable>& sine_table;NinjaSpriteProjection& projection;const std::optional<float>& width_at_3d0f9e4;
    const std::optional<std::array<float,4>>& color_at_3b18230;NinjaQueueColorOwners colors;
    X87Context& fp;
    GameFrameOwnerInvoke invoke;
    // NOTSA: reference validation and source-callee transport around shared caption game steps.
    std::function<GameFrameOwnerResult(const GameFrameOwnerInvoke&)> complete_caption_frame;
};
GameFrameOwnerResult run_game_frame_owner(std::uint32_t callee,std::optional<std::uint32_t> argument,GameFrameOwnersRuntime&);
} // namespace sadx
