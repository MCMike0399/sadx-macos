#include "engine/queue_consumer_native.hpp"
#include "engine/ninja_sprite_queue.hpp"
namespace sadx::queue {
void skip_execution(State &state) { state.values.execution_mode = 1; }
ConsumerResult consume(ConsumerView view, const ConsumerServices &services, X87Context &fp) {
    auto &owners = view.owners;
    auto &state = owners.state;
    const auto saved_control = view.control_3d;
    const auto saved_mask = view.constant_attribute_mask;
    const auto saved_attributes = view.constant_attributes;
    const auto saved_mode = static_cast<std::uint32_t>(state.values.lighting_palette);
    const auto saved_fog = view.fog_active;
    reset_material(owners, services.queue, fp);
    if (owners.loop_count != 0 || state.values.execution_mode != 0) {
        if (state.values.execution_mode == 2) {
            if (!services.queue.set_depth_comparison || !services.queue.set_depth_comparison(7))
                throw Unavailable(Boundary::DepthComparison);
            if (!services.draw_skipped_frame)
                return {ConsumerBoundary::SkippedFrame, {}, 0, false};
            services.draw_skipped_frame();
        }
        state.values.execution_mode = 1;
        return {ConsumerBoundary::None, {}, 0, true};
    }
    state.values.execution_mode = 1;
    if (state.values.primary_high_water < state.values.primary_count)
        state.values.primary_high_water = state.values.primary_count;
    if (state.values.secondary_high_water < state.values.secondary_count)
        state.values.secondary_high_water = state.values.secondary_count;
    if (state.values.lighting_palette != 0) {
        state.values.lighting_palette = 0;
        state.values.lighting_copy = 0;
    }
    if (!services.capture_matrix)
        return {ConsumerBoundary::Matrix, {}, 0, false};
    const auto saved_matrix = services.capture_matrix();
    auto pass = (view.special_flags & 3) == 3 ? 1 : 0;
    for (; pass >= 0; --pass)
        for (std::size_t bucket = 0; bucket < 2048; ++bucket) {
            const auto front = owners.storage.records().bucket_front(bucket);
            if (!front)
                return {ConsumerBoundary::Bucket, {}, bucket, false};
            if (front.record)
                return {ConsumerBoundary::QueuedRecord, front.record, bucket, false};
        }
    if (!services.restore_matrix)
        return {ConsumerBoundary::Matrix, {}, 0, false};
    services.restore_matrix(saved_matrix);
    view.queued_texture_list = nullptr;
    set_material(state.current_material, state, services.queue, fp);
    state.values.lighting_palette = static_cast<std::int32_t>((saved_mode >> 1) & 63U);
    state.values.lighting_copy = static_cast<std::uint8_t>(state.values.lighting_palette);
    if (!services.queue.set_depth_comparison || !services.queue.set_depth_comparison(1))
        throw Unavailable(Boundary::DepthComparison);
    if (!services.queue.set_depth_write || !services.queue.set_depth_write(true))
        throw Unavailable(Boundary::DepthWrite);
    view.constant_attribute_mask = saved_mask;
    view.constant_attributes = saved_attributes;
    view.control_3d = saved_control;
    if (!services.restore_fog)
        throw Unavailable(Boundary::Device);
    services.restore_fog(saved_fog != 0);
    return {ConsumerBoundary::None, {}, 0, true};
}
} // namespace sadx::queue
