#include "engine/particle_pool.hpp"
#include <cstring>

namespace sadx::particles {
namespace {
// NOTSA: explicit sign extension avoids implementation-defined unsigned conversion.
std::int32_t signed_count(std::uint16_t value) {
    constexpr std::int32_t kIndexModulus = 65536;
    return value < 32768 ? value : std::int32_t(value) - kIndexModulus;
}
std::shared_ptr<Work> live_work(const TaskHandle &task) {
    if (!task || !task->auxiliary_native_work)
        throw Failure(Operation::Work);
    return std::static_pointer_cast<Work>(task->auxiliary_native_work);
}
std::uint16_t read_index(TaskWorkArena &arena, const TaskWorkHandle &buffer, std::int32_t index) {
    std::uint16_t result;
    const auto read = arena.read(buffer, std::uint32_t(index) * 2, &result, sizeof(result));
    if (!read)
        throw Failure(Operation::ActiveIndexRead, Field::PositionX, read.failure);
    return result;
}
void write_index(TaskWorkArena &arena, const TaskWorkHandle &buffer, std::int32_t index,
                 std::uint16_t value, Operation operation) {
    const auto write = arena.write(buffer, std::uint32_t(index) * 2, &value, sizeof(value));
    if (!write)
        throw Failure(operation, Field::PositionX, write.failure);
}
Particle active_particle(TaskWorkArena &arena, Work &work, std::int32_t index) {
    const auto slot = signed_count(read_index(arena, work.active_indices, index));
    return {arena, work.particles, std::uint32_t(slot) * 48};
}
// NOTSA: shared source removal tail; reloads task.awp independently of loop work.
void remove_particle(TaskWorkArena &arena, const TaskHandle &task, std::int32_t index) {
    const auto work = live_work(task);
    if (signed_count(work->active_count) <= 0)
        return;
    const auto active = work->active_indices;
    const auto free = work->free_indices;
    const auto last_index = static_cast<std::uint16_t>(work->active_count - 1);
    work->active_count = last_index;
    const auto value = read_index(arena, active, index);
    write_index(arena, free, signed_count(last_index), value, Operation::FreeIndexStore);
    const auto replacement = work->active_indices;
    const auto last = read_index(arena, replacement, signed_count(work->active_count));
    write_index(arena, replacement, index, last, Operation::ActiveIndexStore);
}
// NOTSA: preserve the source's 32-bit descriptor-index multiplication, including
// wrap back into a valid imported table. No animation data is copied.
Frame &indexed_frame(Frame *frames, std::uint32_t index) {
    if (!frames)
        throw Failure(Operation::FrameRead);
    const std::uint32_t byte_offset = index * sizeof(Frame);
    std::int32_t displacement;
    std::memcpy(&displacement, &byte_offset, sizeof(displacement));
    return *reinterpret_cast<Frame *>(reinterpret_cast<char *>(frames) + displacement);
}
fidelity::Context &arithmetic(TaskScheduler &scheduler) {
    const auto binding = std::dynamic_pointer_cast<Binding>(scheduler.callback_context(update_task));
    if (!binding)
        throw Failure(Operation::ArithmeticBinding);
    return binding->arithmetic;
}
} // namespace

Failure::Failure(Operation value, Field member, TaskWorkFailure reason)
    : std::runtime_error("native_particle_lifecycle_boundary"), operation(value), field(member),
      cause(reason) {}

std::uint32_t Particle::word(Field field) const {
    std::uint32_t value;
    const auto result = arena.read(storage, byte_offset + static_cast<std::uint32_t>(field),
                                   &value, sizeof(value));
    if (!result)
        throw Failure(Operation::ParticleRead, field, result.failure);
    return value;
}
void Particle::word(Field field, std::uint32_t value) {
    const auto result = arena.write(storage, byte_offset + static_cast<std::uint32_t>(field),
                                    &value, sizeof(value));
    if (!result)
        throw Failure(Operation::ParticleStore, field, result.failure);
}
float Particle::scalar(Field field) const {
    const auto value = word(field);
    float result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}
void Particle::scalar(Field field, float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, sizeof(result));
    word(field, result);
}

std::int32_t update_animation(Particle &particle, Frame *frames, fidelity::Context &fp) {
    const auto frame_index = particle.word(Field::FrameIndex);
    // Source integer increment/multiply wrap before the frame-table access.
    const auto next_index = frame_index + 1;
    auto &next = indexed_frame(frames, next_index);
    if (next.texture_index < 0)
        return -1;
    const auto speed = fidelity::ExactFloat(fp, particle.scalar(Field::AnimationSpeed));
    const auto time = (speed + particle.scalar(Field::AnimationTime)).rounded();
    const auto elapsed = fidelity::ExactFloat(fp, time);
    particle.scalar(Field::AnimationTime, time);
    const auto comparison = elapsed.compare(next.parameters[0]);
    if (comparison == X87Comparison::less || comparison == X87Comparison::unordered)
        return 0;
    auto &following = indexed_frame(frames, next_index + 1);
    if (following.texture_index == -2) {
        const auto loops = particle.word(Field::LoopCount) + 1;
        particle.word(Field::LoopCount, loops);
        if (loops < static_cast<std::uint32_t>(following.global_index)) {
            const auto remaining = elapsed - next.parameters[0];
            particle.word(Field::FrameIndex, 0);
            particle.scalar(Field::AnimationTime, remaining.rounded());
            return -2;
        }
        particle.scalar(Field::AnimationTime, next.parameters[0]);
        return -1;
    }
    if (following.texture_index == -1) {
        particle.scalar(Field::AnimationTime, next.parameters[0]);
        return -1;
    }
    particle.word(Field::FrameIndex, next_index);
    return 0;
}

void update_task(TaskScheduler &scheduler, const TaskHandle &task) {
    const auto work = live_work(task);
    auto &arena = *scheduler.work_arena();
    for (std::int32_t index = 0; index < signed_count(work->active_count); ++index) {
        auto particle = active_particle(arena, *work, index);
        auto *frames = work->frames;
        auto &fp = arithmetic(scheduler);
        using Exact = fidelity::ExactFloat;
        for (const auto pair : {std::pair{Field::PositionX, Field::VelocityX},
                                std::pair{Field::PositionY, Field::VelocityY},
                                std::pair{Field::PositionZ, Field::VelocityZ}}) {
            const auto position = Exact(fp, particle.scalar(pair.first));
            particle.scalar(pair.first, (position + particle.scalar(pair.second)).rounded());
        }
        const auto drag_x = Exact(fp, work->parameters.drag) * particle.scalar(Field::VelocityX);
        particle.scalar(Field::VelocityX,
                        (particle.scalar(Field::VelocityX) - drag_x).rounded());
        const auto drag_y = Exact(fp, work->parameters.drag) * particle.scalar(Field::VelocityY);
        const auto falling = drag_y - work->parameters.vertical_acceleration;
        particle.scalar(Field::VelocityY,
                        (particle.scalar(Field::VelocityY) - falling).rounded());
        const auto drag_z = Exact(fp, work->parameters.drag) * particle.scalar(Field::VelocityZ);
        particle.scalar(Field::VelocityZ,
                        (particle.scalar(Field::VelocityZ) - drag_z).rounded());
        if (update_animation(particle, frames, fp) == -1) {
            remove_particle(arena, task, index);
            --index;
        }
    }
    display_task(scheduler, task);
}
void update_custom_task(TaskScheduler &scheduler, const TaskHandle &task) {
    const auto work = live_work(task);
    auto &arena = *scheduler.work_arena();
    for (std::int32_t index = 0; index < signed_count(work->active_count); ++index) {
        auto particle = active_particle(arena, *work, index);
        auto *frames = work->frames;
        const auto update = work->update;
        if (!update)
            throw Failure(Operation::CustomUpdate);
        if (update(particle, work->parameters, frames) == 0) {
            remove_particle(arena, task, index);
            --index;
        }
    }
    display_task(scheduler, task);
}
void display_task(TaskScheduler &scheduler, const TaskHandle &task) {
    const auto work = live_work(task);
    auto &arena = *scheduler.work_arena();
    for (std::int32_t index = 0; index < signed_count(work->active_count); ++index) {
        auto particle = active_particle(arena, *work, index);
        auto *frames = work->frames;
        display_particle(scheduler, particle, frames);
    }
}
void destroy_task(TaskScheduler &scheduler, const TaskHandle &task) {
    const auto work = live_work(task);
    auto &arena = *scheduler.work_arena();
    const auto free = work->free_indices;
    release_buffer(arena, free);
    const auto active = work->active_indices;
    release_buffer(arena, active);
    const auto particles = work->particles;
    release_buffer(arena, particles);
}
} // namespace sadx::particles
