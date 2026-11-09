#pragma once

#include "engine/ninja_sprite_queue.hpp"
#include "cpu/fidelity_float.hpp"

#include <cstdint>
#include <functional>

namespace sadx {
enum class LateModelRoute { queued, immediate };

// The allocated node, the depth handed to the sorter, and whether the blob was
// queued or left for the unrecovered immediate draw.
struct LateModelResult {
    NinjaQueueFailure failure = NinjaQueueFailure::none;
    std::uint32_t source_boundary = 0;
    LateModelRoute route = LateModelRoute::immediate;
    NinjaQueueRecordHandle record;
    std::optional<std::size_t> bucket;
    float depth = 0;
    explicit operator bool() const noexcept { return failure == NinjaQueueFailure::none; }
};

// FUNCTION: SONIC 0x4048e0 lateDrawModel
LateModelResult late_draw_model(NinjaSpriteQueue &queue, const NinjaMatrix &matrix,
                                std::uint32_t model_word, std::uint8_t late_flags, float depth_limit,
                                std::function<void()> draw, NinjaSpriteQueueRuntime &runtime,
                                fidelity::Context &arithmetic);

// A linked type-1 node is drawn or skipped here. Any other type stays with the caller.
bool dispatch_queued_model(const NinjaQueueRecordHandle &record, bool simple_models,
                           std::int32_t lighting_palette,
                           const std::function<void(const NinjaQueueRecordHandle &, const NinjaMatrix &,
                                                    std::uint32_t, bool)> &draw);
} // namespace sadx
