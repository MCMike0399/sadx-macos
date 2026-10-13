#pragma once
#include <optional>
#include "engine/game_logic.hpp"
#include "engine/ninja_keyboard.hpp"
#include "engine/ninja_memory.hpp"
#include "engine/ninja_frame_calls.hpp"
#include "engine/ninja_texture_pool.hpp"
#include "cpu/source_bytes.hpp"
#include "cpu/x87_math.hpp"
#include <array>
#include <cstdint>
#include <functional>
namespace sadx {
// 401830 (system init, WinMain 402c8d) from exact instructions, with its leaf
// callees ported here: 77f240, 780ea0, 780870, 7808c0, 780850 -> 78e720,
// 7803b0 (ECX 38f6ef8), 77f610 (ninja_texture_pool), 780140, 77ff60, 77f150,
// 77efd0 and 40bb20. 780180 and 77f0a0 are owners already ported elsewhere;
// 402f10/780c00 writes the borrowed clear colours. Device, input and
// allocation work the source delegates (794540, 793c70, 7928e0, 785240, 791d60,
// DirectInput8 COM) are owners; the 77f110/77efc0 callbacks join NinjaCallbackList.
struct NinjaSystemInitState {
    std::optional<std::uint32_t> word_at_3d0f83c,word_at_3d0f810;           // 77f240
    std::optional<std::uint32_t> pool_at_3ce71b0,size_at_3ce71a4;           // 780ea0
    std::optional<std::uint32_t> color_at_3ce715c,buffer_at_3ce7158;        // 780870
    std::optional<float> scale_at_3ce7154;                                   // 780870 / 7808c0
    std::optional<std::uint32_t> word_at_3d127a0;                            // 78e720
    std::optional<std::uint32_t> word_at_3d0f740;                            // 7803b0
    std::optional<std::uint32_t> flag_at_3ce7150,counter_low_at_3ce7130,counter_high_at_3ce7134; // 780140
    SourceBytes<0x3ce6f48,0x1e0> records_at_3ce6f48;                         // 77f150: 4 x 0x78
    SourceBytes<0x3ce6e78,0xd0> records_at_3ce6e78;                          // 77f150: 4 x 0x34
    // 77efd0: 4 x 0x18 records at 3ce6d80 (each also writes +0x16..+0x1a, so the last spills into
    // 3ce6de0..3ce6de2, the object 791d60 receives) and 4 x 0x12 records at 3ce6df0: one block.
    SourceBytes<0x3ce6d80,0xb8> block_at_3ce6d80;
    std::optional<std::uint32_t> direct_input_at_3abdc10,mouse_at_3abdc14;  // 40bb20
    std::array<std::optional<std::uint32_t>,27> tables_at_389d888;          // 78e720's 794540 argument tables (389d888, 389d8ac, 389d8d0)
};
struct NinjaSystemInitBorrowed {
    NinjaMemoryState& memory;GuestHeap& heap;  // njMalloc
    GuestHeap& crt_heap;                       // the CRT's malloc 645002
    NinjaFrameCallState& frame;                // 3d128ac
    SourceBytes<0x3d0fae0,0x200>& block_at_3d0fae0; // NinjaDeviceState
    const std::array<std::optional<std::uint32_t>,9>& sizes_at_389d864; // GameLibraryInitState
    NinjaTexturePoolState& textures;NinjaTextureSelectionState& selection;
    NinjaCallbackList& callbacks;
    std::optional<std::uint32_t>& word_at_3d0f9c8;   // NinjaSpriteQueueRuntime (77ff60)
    std::array<std::optional<std::uint32_t>,4>& entry_words_at_3d08428; // ResetLeafState (785240)
    std::array<std::optional<std::uint32_t>,4>* clear_colors_at_3ce7174; // GameViewState (402f10 -> 780c00)
    const std::optional<std::uint32_t>& window_at_3d0fd30;const std::optional<std::uint32_t>& instance_at_3d0fd34; // set by window creation
};
// The pinned Input mod's Init patches inside these callees (input-mod.cpp): 77f0d7 (77f0a0's 791b10)
// -> CreateSADXKeyboard (3b0e340 = its SADXKeyboard), and NOPs over 77f03e (77efd0's 791d60) and
// 77f205 (77f150's 7928e0). 77f0a0 itself is the runtime's owner.
struct NinjaSystemCall {std::uint32_t source_call,callee;std::optional<std::uint32_t> ecx,edx;std::array<std::optional<std::uint32_t>,4> arguments={};};
struct NinjaSystemOwnerResult {GameLogicOwnerResult owner;std::optional<std::uint32_t> eax;};
struct NinjaSystemInitRuntime {
    NinjaSystemInitState& state;NinjaSystemInitBorrowed borrowed;
    X87Context& fp;
    // 780180 (njInitMatrix), 77f0a0 (keyboard init), 794540, 793c70, 7928e0, 785240, 791d60,
    // 7db098 QueryPerformanceCounter (EAX, the 64-bit count in arguments[0..1] on return is not modelled: use counter()),
    // the DirectInput8 owners: 77dbb0 DirectInput8Create (EAX = the interface written to 3abdc10) and the COM
    // methods named by their call sites 40bb4f CreateDevice (EAX = device), 40bb5f, 40bb71, 40bb7c.
    std::function<NinjaSystemOwnerResult(const NinjaSystemCall&)> invoke;
    std::function<std::optional<std::int64_t>()> counter; // QueryPerformanceCounter for 780140
};
enum class NinjaSystemInitFailure {none,unknown_state,missing_owner,owner_failed,cancelled,memory_failed,store_outside_owner,fp_failed};
struct NinjaSystemInitResult {
    NinjaSystemInitFailure failure=NinjaSystemInitFailure::none;std::uint32_t source_boundary=0;std::optional<std::uint32_t> required_fact;
    std::optional<NinjaSystemCall> pending_call;
    explicit operator bool()const noexcept{return failure==NinjaSystemInitFailure::none;}
};
NinjaSystemInitResult run_system_init_401830(NinjaSystemInitRuntime&);
// 77f150 alone (781070 calls it before 401830 does): the record resets, the Input mod's NOP over
// 7928e0, the 77f110 registration and its 785240 tail (3d08428 + 0x18i = 0, then 785190 registered).
NinjaSystemInitResult run_controllers_77f150(NinjaSystemInitRuntime&);
} // namespace sadx
