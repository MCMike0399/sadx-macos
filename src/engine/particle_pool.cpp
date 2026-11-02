#include "engine/particle_pool.hpp"
#include <stdexcept>

namespace sadx::particles {
namespace {
constexpr std::uint8_t kLoadTextures = 1;
constexpr std::uint8_t kCopyTextureDimensions = 1;
constexpr std::uint8_t kCenterFrame = 2;
} // namespace
void bind_frames(texture_lifetime::List &textures, Frame *frame, std::uint8_t flags,
                 fidelity::Context &arithmetic) {
    if (flags & kLoadTextures)
        load_textures(textures);
    for (; frame->texture_index >= 0; ++frame) {
        // NOTSA: malformed imported/live indices fail at the actual lookup boundary.
        auto *memory = textures.entries.at(static_cast<std::size_t>(frame->texture_index));
        if (!memory)
            continue;
        frame->global_index = memory->global_index;
        if (frame->flags & kCopyTextureDimensions) {
            // Source converts signed first, then adds 2^32 for unsigned dimensions.
            const auto width = static_cast<std::int32_t>(memory->width);
            auto exact_width = fidelity::ExactFloat::integer(arithmetic, width);
            if (width < 0)
                exact_width = exact_width + 4294967296.0f;
            frame->width = exact_width.rounded();
            const auto height = static_cast<std::int32_t>(memory->height);
            auto exact_height = fidelity::ExactFloat::integer(arithmetic, height);
            if (height < 0)
                exact_height = exact_height + 4294967296.0f;
            frame->height = exact_height.rounded();
        }
        if (frame->flags & kCenterFrame) {
            frame->center_x = (fidelity::ExactFloat(arithmetic, frame->width) * 0.5f).rounded();
            frame->center_y = (fidelity::ExactFloat(arithmetic, frame->height) * 0.5f).rounded();
        }
    }
}

TaskHandle create_pool(TaskScheduler &scheduler, TaskWorkArena &arena, std::int32_t count,
                       const Parameters &parameters, Frame *frames, Update update) {
    const auto task = create_task(scheduler);
    if (!task)
        return {};
    const auto allocation = allocate_buffer(arena, 32);
    if (!allocation) {
        discard_task(scheduler, task);
        return {};
    }
    auto work = std::make_shared<Work>();
    work->allocation = allocation;
    // Publish before buffer allocation: lower allocations may reenter this task.
    task->auxiliary_work = allocation;
    task->auxiliary_native_work = work;
    work->active_count = 0;
    work->capacity = static_cast<std::uint16_t>(count);
    const auto index_bytes = static_cast<std::uint32_t>(count) * 2;
    work->free_indices = allocate_buffer(arena, index_bytes);
    work->active_indices = allocate_buffer(arena, index_bytes);
    work->particles = allocate_buffer(arena, static_cast<std::uint32_t>(count) * 48);
    work->parameters.drag = parameters.drag;
    const auto acceleration = parameters.vertical_acceleration;
    work->frames = frames;
    work->parameters.vertical_acceleration = acceleration;
    work->update = update;
    if (update)
        task->exec = update_custom_task;
    for (std::int32_t index = 0; index < count; ++index) {
        const auto value = static_cast<std::uint16_t>(index);
        if (!arena.write(work->free_indices, static_cast<std::uint32_t>(index) * 2,
                         &value, sizeof(value)))
            throw std::runtime_error("particle_pool_free_index_store");
    }
    task->display = display_task;
    task->destructor = destroy_task;
    return task;
}
} // namespace sadx::particles
