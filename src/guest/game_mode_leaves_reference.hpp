#pragma once
#include "guest/frame_control_reference.hpp"
#include "guest/save_name_reference.hpp"
#include "cpu/crt_rand.hpp"
#include "engine/game_logic.hpp"
#include "guest/game_mode_reference.hpp"
#include "game/model_dlls.hpp"
#include "cpu/x87_math.hpp"
#include "cpu/source_bytes.hpp"
#include "engine/task.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
namespace sadx {
// Owners reached from the 40c090 game-mode machine, each a direct translation
// of exact instructions. Words without another native owner live here; shared
// words are borrowed. Calls to unported code go through `nested`.
struct GameModeLeafState {
    GameModeLeafState(GameModeState& m,GameLogicTimingState& t,std::optional<std::int32_t>& cutscene,std::optional<TaskHandle>& event,CrtRandState& r,
        std::optional<std::uint16_t>& game_mode,std::optional<std::uint16_t>& stage)
        :mode(m),timing(t),demo_cutscene_at_3b2a2e8(cutscene),event_task_at_3b2c578(event),rng(r),game_mode_at_3b22de4(game_mode),stage_at_3b22dcc(stage){save_name_reference.import_value(save_name,std::nullopt);}
    GameModeState& mode;                                   // 3b290e4, 3b2a2fa, level pointer 3b0ef4c
    GameLogicTimingState& timing;                          // 3b11178 frame delta, 3b2a2e4 demo
    std::optional<std::int32_t>& demo_cutscene_at_3b2a2e8; // InputModFrameState
    std::optional<TaskHandle>& event_task_at_3b2c578;      // PlayerScenePositionGlobals
    CrtRandState& rng;
    std::optional<std::uint16_t>& game_mode_at_3b22de4; // GameLoopState
    std::optional<std::uint16_t>& stage_at_3b22dcc;     // SonicPlayerGlobals
    std::optional<std::uint16_t> character_at_3b22dc0;
    std::optional<std::uint8_t> byte_at_3b0ef34,byte_at_3b0ef35,byte_at_3b0f128,byte_at_3b0ef48,byte_at_3abe134,byte_at_3b0f0e0;
    std::optional<std::uint32_t> word_at_3b0f13c,word_at_3b0f108,word_at_3b0ef44;
    std::optional<std::uint16_t> half_at_3b0f0e4;
    // 421f90
    std::optional<std::uint8_t> byte_at_3b290d8,byte_at_3b290e0,byte_at_3b290e8;save::SaveNameState save_name;
    SaveNameReference save_name_reference;
    // 42cea0: the dword at 3b2a2f8 spans these bytes and the borrowed 3b2a2fa
    std::optional<std::uint32_t> word_at_3b2a2f0,word_at_3b2a2f4,word_at_3b2a2fc,word_at_3b2b358;
    std::optional<std::uint8_t> byte_at_3b2a2f8,byte_at_3b2a2f9,byte_at_3b2a2fb;std::optional<std::uint16_t> half_at_3b2a300;
    // 42c7b0
    std::optional<std::uint32_t> word_at_3b2c47c,word_at_3b2919c;
    // 43b490
    std::optional<std::uint32_t> word_at_3b0f0f8,word_at_3b0f0fc,word_at_3b0f100,word_at_3b0f10c,word_at_3b0f0f4;
    // 42f5d0 / 413300
    std::optional<std::uint32_t> word_at_3b2c560,word_at_914788,word_at_3b18244;std::optional<std::uint8_t> byte_at_3b2c565,byte_at_3b2c574;
    SourceBytes<0x3b18250,0xb64> block_at_3b18250;   // zeroed tables, 3b18849, 3b18db0
    SourceBytes<0x3b0ef9c,0x3c> block_at_3b0ef9c;    // 427ec0
    SourceBytes<0x3c4ae08,0x3638> block_at_3c4ae08;  // 46b230 (414470's tail)
    SourceBytes<0x3b290c4,8> block_at_3b290c4;       // 420e90 per-character counts
    SourceBytes<0x90ed54,0x20> block_at_90ed54;      // 420e90 per-character release lists
    SourceBytes<0x3b2b5f9,7> block_at_3b2b5f9;       // 413c00 flag byte, then 414470's per-character bytes
    // 413c00 / 783260
    std::optional<std::uint16_t> half_at_3b22e1c,half_at_3b18db8;std::optional<std::uint32_t> word_at_3b22e20,callback_at_3d08420;
    // 422680 per-character motion words copied out of the bound character data
    std::optional<std::uint32_t> motion_at_3c566c0,motion_at_3c566d0,motion_at_3c559c0,motion_at_3c559d0,motion_at_3c559e0,motion_at_3c559f0,motion_at_3c55a00,motion_at_3c53510,motion_at_3c53520;
    // Borrowed model-DLL bindings (401ac0) and images; without them 422680 is a nested owner.
    const ModelDllBindings* model_bindings=nullptr;const ModelDllSet* model_dlls=nullptr;
    // 42c4f0 fade overlay: colour dword 3b29d5c (its top byte 3b29d5f gates the draw);
    // screen scale (GameViewState) and the caller's x87 context are borrowed.
    std::optional<std::uint32_t> color_at_3b29d5c;
    // 42c730 (SEGA logo setup): 42e470's word, the fade-machine state 3b29d6c/3b29d68, 432ea0's
    // run 3b2c5e8..3b2c65f; the 780c00 clear colours 3ce7174..80 are GameViewState's (borrowed;
    // without them 402f10 is a nested owner).
    std::optional<std::uint32_t> word_at_3b2c420,state_at_3b29d6c,word_at_3b29d68;
    SourceBytes<0x3b2c5e8,0x78> block_at_3b2c5e8;
    std::array<std::optional<std::uint32_t>,4>* clear_colors_at_3ce7174=nullptr;
    const std::optional<float>* screen_width_at_8928c0=nullptr;const std::optional<float>* screen_height_at_8928c4=nullptr;X87Context* fp=nullptr;
};
// Arguments are the cdecl stack words in order (floats as their bit patterns).
struct GameModeLeafCall {std::uint32_t source_call,callee;std::array<std::optional<std::uint32_t>,7> arguments={};};
using GameModeLeafNested=std::function<GameLogicOwnerResult(const GameModeLeafCall&)>;
// nonterminating: the source loops forever there (422680's default case).
enum class GameModeLeafOutcome {not_a_leaf,completed,unknown_state,nested_failed,nonterminating};
struct GameModeLeafResult {
    GameModeLeafOutcome outcome=GameModeLeafOutcome::not_a_leaf;std::uint32_t source_boundary=0;std::optional<std::uint32_t> required_fact;
    std::optional<std::uint32_t> eax;GameLogicOwnerResult nested{};
};
// Runs the leaf for `callee` (with its first stack argument) if it is ported.
GameModeLeafResult run_game_mode_leaf(std::uint32_t callee,std::optional<std::uint32_t> argument,GameModeLeafState&,const GameModeLeafNested&);
} // namespace sadx
