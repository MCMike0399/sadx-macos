#pragma once
#include "engine/game_logic.hpp"
#include "game/saved_fov.hpp"
#include "engine/ninja_queue_lifecycle.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>

namespace sadx {
// 4208e0 view setup (called from 40bf40) and its 420860 conditional reset.
struct GameViewState {
    std::optional<float> screen_width_at_8928c0,screen_height_at_8928c4;
    float scale_x_at_7dcb28,scale_y_at_7dcb24,half_at_7dcc98,fov_x_at_7dd594,fov_y_at_7dd590; // .rdata
    std::optional<std::uint8_t>& reset_pending_at_3b280a8;  // borrowed (420830/420840 owner)
    std::optional<std::uint32_t>& mode_at_3b28114;          // borrowed (GameLoopState)
    std::optional<std::uint32_t> word_at_3abd9cc;
    // struct at 3b28094: +0 scale, +4 w, +8 h, +0xc w*k, +0x10 h*k
    std::optional<std::uint32_t> scale_word_at_3b28094;
    std::optional<float> view_at_3b28098,view_at_3b2809c,view_at_3b280a0,view_at_3b280a4;
    std::optional<std::uint32_t> projection_words_at_3d0f9ec,projection_words_at_3d0f9f0; // 788240 copies of +0xc/+0x10
    std::optional<std::uint32_t> fov_at_3aad0bc,fov_copy_at_3ab98ec;
    view::SavedFovState& saved_fov;
    // NOTSA: reference transport can observe this write without owning another word.
    std::function<void()> observe_saved_fov_write;
    std::array<std::optional<std::uint32_t>,4> clear_colors_at_3ce7174; // 780c00
    const std::optional<std::array<float,4>>& color_at_3b280ac;
};
struct GameViewCall {std::uint32_t source_call,callee;std::array<std::optional<std::uint32_t>,6> arguments={};};
struct GameViewRuntime {
    GameViewState& view;
    NinjaQueueLifecycleOwners& queue;     // 407ab0, 402f40, render owner, captured words, projection
    std::uint32_t queue_source_base=0;    // 3abd960 for 407ab0's alignment
    std::function<GameLogicOwnerResult(const GameViewCall&)> invoke; // 4071c0, 7815c0, 403180
};
enum class GameViewFailure {none,unknown_state,render_failed,fp_failed,queue_failed,missing_owner,owner_failed};
struct GameViewResult {
    GameViewFailure failure=GameViewFailure::none;std::uint32_t source_boundary=0;
    std::optional<GameViewCall> pending_call;NinjaRenderResult render;bool completed=false;
    explicit operator bool()const noexcept{return failure==GameViewFailure::none&&completed;}
};
GameViewResult run_game_view_4208e0(GameViewRuntime&,X87Context&);
} // namespace sadx
