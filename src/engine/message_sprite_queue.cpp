#include "engine/message_sprite_queue.hpp"
#include "cpu/fidelity_float.hpp"
#include <new>
namespace sadx::queue {
AnimationAllocation allocate_animation(Storage &storage, std::uint32_t bytes,
                                       const NinjaQueueLogger &logger) {
    static_assert(sizeof(sprite::AnimationFrame) == 20);
    if (bytes != sizeof(sprite::AnimationFrame))
        throw std::invalid_argument("ANIM allocation size differs from source20bytes");
    const auto arena = storage.buffer().arena;
    if (!arena)
        throw Unavailable(Boundary::Buffer);
    const auto allocation = arena->allocate(bytes, logger);
    if (!allocation || !allocation.block)
        throw Unavailable(Boundary::Buffer);
    auto *data = arena->mutable_block(*allocation.block);
    auto *frame = ::new (static_cast<void *>(data)) sprite::AnimationFrame;
    return {{arena, data, bytes}, frame};
}
ArenaBytes borrow_from_end(Storage &storage, std::uint32_t size) {
    const auto arena = storage.buffer().arena;
    if (!arena)
        throw Unavailable(Boundary::Buffer);
    return {arena, arena->borrow_from_end(size), size};
}
float prepare_immediate_priority(std::uint8_t flags, float priority, LateSpriteView view,
                                 const LateSpriteServices &services, X87Context &fp) {
    const auto compared = fidelity::ExactFloat(fp, priority).compare(10000.0f);
    const bool nearer = compared == X87Comparison::less || compared == X87Comparison::unordered;
    const auto depth_mode = nearer && !(flags & 16) ? 3U : 7U;
    if (!services.queue.set_depth_comparison || !services.queue.set_depth_comparison(depth_mode))
        throw Unavailable(Boundary::DepthComparison);
    const bool write = depth_mode == 3 && (flags & 1) != 0;
    if (!services.queue.set_depth_write || !services.queue.set_depth_write(write))
        throw Unavailable(Boundary::DepthWrite);
    if (!services.copy_device_material)
        throw Unavailable(Boundary::Material);
    if (flags & 2) {
        if (!services.secondary_material)
            throw Unavailable(Boundary::Material);
        services.copy_device_material(services.secondary_material());
    } else
        services.copy_device_material(view.owners.state.current_material);
    const auto lower = fidelity::ExactFloat(fp, priority).compare(-1.0f);
    return fidelity::ExactFloat(fp, lower == X87Comparison::greater ? -1.0f : priority).rounded();
}
LateResult draw_late_sprite(const sprite::ScreenSprite *sprite, std::int32_t frame, float priority,
                            std::uint32_t attributes, std::uint32_t queue_flags,
                            LateSpriteView view, const LateSpriteServices &services,
                            X87Context &fp) {
    if (view.owners.loop_count != 0 || !sprite)
        return {LateRoute::NotDrawn, true};
    const auto *list = sprite->textures;
    if (!list || list->entries.empty())
        return {LateRoute::NotDrawn, true};
    for (const auto *entry : list->entries)
        if (!entry)
            return {LateRoute::NotDrawn, true};
    if (list->first_filename_invalid)
        return {LateRoute::NotDrawn, true};
    auto adjusted = priority;
    const auto lower = fidelity::ExactFloat(fp, priority).compare(-2.0f);
    if (lower == X87Comparison::equal || lower == X87Comparison::greater) {
        if (fidelity::ExactFloat(fp, priority).compare(10000.0f) == X87Comparison::less)
            adjusted = (fidelity::ExactFloat(fp, priority) + 12048.0f).rounded();
    }
    const auto flags = static_cast<std::uint8_t>(queue_flags);
    // Actual403f60 gate: nonzeroexecutionmode or flag8 returnsNULL. Mode0
    // eligibleallocation is NOTpretendedNULL/immediate: native type2capture is
    // explicitly unrecovered before403d30/recordheader effects.
    if (view.owners.state.values.execution_mode == 0 && !(flags & 8))
        return {LateRoute::NeedsType2Record, false};
    adjusted = prepare_immediate_priority(flags, adjusted, view, services, fp);
    if (!services.draw)
        throw Unavailable(Boundary::Device);
    services.draw(*sprite, frame, adjusted, attributes);
    if (!services.restore_default_depth)
        throw Unavailable(Boundary::DepthComparison);
    services.restore_default_depth();
    if (!services.queue.set_depth_write || !services.queue.set_depth_write(true))
        throw Unavailable(Boundary::DepthWrite);
    reset_material(view.owners, services.queue, fp);
    return {LateRoute::Immediate, true};
}
} // namespace sadx::queue
