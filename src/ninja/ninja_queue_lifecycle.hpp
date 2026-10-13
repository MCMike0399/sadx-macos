#pragma once
#include "game/game_logic_support.hpp"
#include "ninja/ninja_queue_consumer.hpp"
#include <array>
#include <cstdint>
#include <optional>

namespace sadx {
// Render-queue lifecycle: 408630 (reallocate the 0x88000-byte buffer, then
// 407ab0) and 407ab0 (reset buffer, 4128f0 -> 402f40 colour, 407330, clear the
// display switch 3b1117c and the counters 3abd9d4/3abd9dc). 407330 = 401420
// (77ed00 with [8928c8]), 77ed20(1), 77ece0(1), 412400(0) -> 401440, bucket
// sentinels, 900840..48 -> 3ab9898..a0, 77ff90(&3ab9864), 3ab98ac=0.
struct NinjaQueueLifecycleOwners {
    NinjaSpriteQueue& queue;
    NinjaSpriteQueueRuntime& queue_runtime;      // 3ab98ac, 3b12208, 3ab9864, render owner
    NinjaQueueConsumerState& consumer;           // 3abdbc0, 3abd9d4, 3abd9dc
    GameLogicTimingState& timing;                // 3b1117c
    NinjaSpriteProjection& projection;           // 3d0f7f0
    const std::optional<std::array<float,4>>& initial_color_at_3b18230;
    const std::array<std::uint32_t,8>* depth_table_at_88ad9c;
    float one_at_7dcb10,negative_one_at_7dcb0c;
    const std::optional<std::uint32_t>& depth_mode_at_8928c8;
    const std::array<std::optional<std::uint32_t>,3>& words_at_900840;
    std::array<std::optional<std::uint32_t>,3>& words_at_3ab9898;
    const std::array<std::optional<std::uint32_t>,3>& words_at_900858;
    std::array<std::optional<std::uint32_t>,3>& words_at_90084c;
};
enum class NinjaQueueLifecycleFailure {none,unknown_fact,render_failed,fp_failed,arena_failed,allocation_failed};
struct NinjaQueueLifecycleResult {
    NinjaQueueLifecycleFailure failure=NinjaQueueLifecycleFailure::none;
    std::uint32_t source_boundary=0;
    std::optional<std::uint32_t> required_fact;
    NinjaRenderResult render;
    NinjaQueueColorResult color;
    std::optional<X87Failure> fp_failure;
    bool alignment_assumed=false,completed=false;
    explicit operator bool()const noexcept{return failure==NinjaQueueLifecycleFailure::none&&completed;}
};
// 407ab0. source_base_address is where the source buffer starts (3abd960);
// only its low five bits matter (the 32-byte cursor alignment).
NinjaQueueLifecycleResult ninja_queue_reset_407ab0(NinjaQueueLifecycleOwners&,std::uint32_t source_base_address,X87Context&);
// 408630. The caller states the HeapAlloc result when it is known; nullopt
// assumes a 32-byte-aligned base and sets alignment_assumed.
NinjaQueueLifecycleResult ninja_queue_allocate_408630(NinjaQueueLifecycleOwners&,std::optional<std::uint32_t> source_heap_address,X87Context&);
} // namespace sadx
