#pragma once
#include "engine/native_texture_lifetime.hpp"
#include "engine/ninja_matrix.hpp"
#include "engine/ninja_queue_native.hpp"
#include "engine/ninja_sprite_queue.hpp"
namespace sadx::queue {
// NOTSA: borrowed canonical native renderer/queue fields. No shadow controls,
// fog/special flags, selected list, matrix or second queue are introduced.
struct ConsumerView {
    Owners owners;
    std::uint32_t &control_3d, &constant_attribute_mask, &constant_attributes;
    const std::uint8_t &special_flags;
    const std::uint8_t &fog_active;
    const texture_lifetime::List *&queued_texture_list;
};
struct ConsumerServices {
    Services queue;
    std::function<NinjaMatrix()> capture_matrix;
    std::function<void(const NinjaMatrix &)> restore_matrix;
    std::function<void(bool)> restore_fog;
    // Source mode2: actual screen-sized4071c0 black rectangle followed401420.
    std::function<void()> draw_skipped_frame;
};
enum class ConsumerBoundary { None, Bucket, QueuedRecord, SkippedFrame, Matrix };
struct ConsumerResult {
    ConsumerBoundary boundary = ConsumerBoundary::None;
    NinjaQueueRecordHandle record;
    std::size_t bucket = 0;
    bool completed = false;
};
// NOTSA: native4086f0 dispatcher prefix/empty-bucket/tail. Actual canonical
// bucket records are inspected; nonempty dispatch remains an explicit boundary
// until its typed native record is recovered. No synthetic empty queue/fallback.
ConsumerResult consume(ConsumerView, const ConsumerServices &, X87Context &);
// FUNCTION: SONIC 0x403e70 FUN_00403e70
void skip_execution(State &);
} // namespace sadx::queue
