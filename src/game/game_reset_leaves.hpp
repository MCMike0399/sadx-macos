#pragma once
#include "game/game_logic.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace sadx {
// Leaf owners reached from 40bf40, each a direct translation of its exact
// instructions. Words without another native owner live here; shared words are
// borrowed. Calls the leaves make to unported code go through `nested`.
struct ResetLeafState {
    // 40cd10
    std::optional<std::uint32_t> word_at_3abdf90,word_at_3abdfac;
    // 401410 / 403e60 (borrowed)
    std::optional<std::uint32_t>& depth_mode_at_8928c8;
    std::optional<std::uint32_t>& queue_mode_at_3ab98ac;
    // 4257c0 / 4257e0
    std::optional<std::uint32_t> word_at_912694,word_at_912698,word_at_91269c,word_at_9126a8,word_at_3b29cf4,word_at_3b29cf8;
    // 421fb0
    std::optional<std::uint32_t> word_at_3b11190,word_at_3c5e8b8;
    std::optional<std::int32_t>& timing_word_at_3b1118c;
    // 4228f0: source callback identity stored at 3b291b4 (0 = none)
    std::optional<std::uint32_t> callback_at_3b291b4;
    // 424460 / 410100
    std::array<std::optional<std::uint32_t>,36>& sound_record_words_at_3b292fc;   // record +0
    std::array<std::optional<std::uint32_t>,36> sound_record_flags_at_3b29304;     // record +8
    std::array<std::optional<std::uint32_t>,36> words_at_3b29b90;
    std::optional<std::uint32_t>& audio_initialized_at_3b0ef2c;
    // 40cff0: audio player objects (source identities) and their flags
    std::optional<std::uint32_t> player_at_3abdf94,player_at_3abdf9c,word_at_3abdfa0,word_at_3abdf98,word_at_3abdfa8;
    // 40d0d0 (via the 4253c0 thunk from 40c030): creates both players when audio
    // is initialized (40ff10 returns 3b0ef2c) and their config words are set.
    std::optional<std::uint32_t> word_at_3abdfa4,config_at_3a7cf3c,config_at_38f6ec0;
    // 40d490
    std::optional<std::uint32_t> word_at_3b0e338;
    // 785330: first word of each 24-byte entry at 3d08428, and the compared word 389d608
    std::array<std::optional<std::uint32_t>,4> entry_words_at_3d08428;
    std::optional<std::uint32_t> word_at_389d608;
    // 422440 / 4224b0: SET and camera file buffers freed (780fb0) and zeroed.
    std::optional<std::uint32_t> word_at_3c4e458,word_at_3c4e45c;
    std::array<std::optional<std::uint32_t>,6> set_files_at_3abdf40,cam_files_at_3abdf28;
    // PrintDebug 401000: vsprintf into 38f6d08, then the 51f680 RET (no output).
    std::string debug_text_at_38f6d08;
};
struct ResetLeafCall {std::uint32_t source_call,callee;std::optional<std::uint32_t> this_object,argument;};
using ResetLeafNested=std::function<GameLogicOwnerResult(const ResetLeafCall&)>;
enum class ResetLeafOutcome {not_a_leaf,completed,unknown_state,nested_failed};
struct ResetLeafResult {ResetLeafOutcome outcome=ResetLeafOutcome::not_a_leaf;std::uint32_t source_boundary=0;std::optional<std::uint32_t> eax;GameLogicOwnerResult nested{};};
// PrintDebug with a constant format (no conversions). Formats containing '%'
// need the unported vsprintf 643dd0 and return false.
bool reset_leaf_print_debug_401000(const std::string& format,ResetLeafState&);
// Runs the leaf for `callee` (with its first stack argument) if it is ported.
ResetLeafResult run_reset_leaf(std::uint32_t callee,std::optional<std::uint32_t> argument,ResetLeafState&,const ResetLeafNested&);
} // namespace sadx
