#include "engine/particle_pool.hpp"
#include <cassert>
#include <iostream>

namespace {
using namespace sadx;
using namespace sadx::particles;
std::int32_t keep_particle(Particle &particle, const Parameters &, Frame *) {
    particle.scalar(Field::PositionY, particle.scalar(Field::PositionY) + 7);
    return 1;
}
std::int32_t remove_particle(Particle &, const Parameters &, Frame *) { return 0; }
}
int main() {
    using namespace sadx;
    using namespace sadx::particles;
    for (const auto count : {0, 1, 40, 32768, 65536, -1}) {
        TaskScheduler scheduler;
        auto arena = std::make_shared<TaskWorkArena>();
        assert(scheduler.initialize_source_pool(arena, {}));
        std::array<Frame, 3> frames{};
        frames[0].texture_index = 0;
        frames[1].texture_index = 1;
        frames[1].parameters[0] = 10;
        frames[2].texture_index = -1;
        const auto slots = scheduler.source_free_slots();
        auto task = create_pool(scheduler, *arena, count, {0.03f, 0}, frames.data(), nullptr);
        assert(task && scheduler.source_free_slots() == slots - 1);
        const auto work = std::static_pointer_cast<Work>(task->auxiliary_native_work);
        const auto buffers = std::array{work->free_indices, work->active_indices, work->particles};
        // Source signed count guard is the only reason empty buffers are not read.
        assert(scheduler.run_index(4));
        assert(scheduler.display_index(4));
        assert(work->frames == frames.data() && work->active_count == 0);
        assert(scheduler.free_task(task));
        assert(!task->retired());
        assert(task->exec == TaskScheduler::destroy_callback);
        // A pending old pool and a newly published pool may coexist.
        const auto next = create_pool(scheduler, *arena, 1, {0.03f, 0}, frames.data(), nullptr);
        assert(next && next != task && !task->retired());
        assert(scheduler.run_index(4));
        assert(task->retired() && !next->retired());
        for (const auto &buffer : buffers) assert(!buffer || buffer->released());
        assert(work->allocation->released() && !task->auxiliary_native_work);
        assert(scheduler.destroy_task(next));
        assert(scheduler.source_free_slots() == slots);
    }
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    assert(scheduler.initialize_source_pool(arena, {}));
    std::array<Frame, 3> frames{};
    frames[0].texture_index = 0; frames[1].texture_index = 1;
    frames[1].parameters[0] = 100; frames[2].texture_index = -1;
    const auto task = create_pool(scheduler, *arena, 4, {0.125f, -0.5f}, frames.data(), nullptr);
    const auto work = std::static_pointer_cast<Work>(task->auxiliary_native_work);
    const std::uint16_t indices[] = {2, 0, 3, 1};
    assert(arena->write(work->active_indices, 0, indices, sizeof(indices)));
    assert(arena->fill(work->particles, 0, 0, 192));
    Particle particle{*arena, work->particles, 96};
    particle.scalar(Field::PositionX, 11); particle.scalar(Field::PositionY, -17);
    particle.scalar(Field::PositionZ, 23); particle.scalar(Field::VelocityX, 2);
    particle.scalar(Field::VelocityY, -4); particle.scalar(Field::VelocityZ, 8);
    particle.scalar(Field::AnimationSpeed, 0.25f);
    work->active_count = 1;
    fidelity::Context arithmetic{127};
    auto binding = std::make_shared<Binding>(arithmetic);
    assert(scheduler.register_callback_context(update_task, binding));
    bool stopped = false;
    try { update_task(scheduler, task); }
    catch (const Failure &failure) { stopped = failure.operation == Operation::DisplayBinding; }
    assert(stopped && work->active_count == 1);
    assert(particle.scalar(Field::PositionX) == 13 && particle.scalar(Field::PositionY) == -21 &&
           particle.scalar(Field::PositionZ) == 31);
    assert(particle.scalar(Field::VelocityX) == 1.75f && particle.scalar(Field::VelocityY) == -4 &&
           particle.scalar(Field::VelocityZ) == 7 && particle.scalar(Field::AnimationTime) == 0.25f);
    unsigned displayed = 0;
    binding->display = [&](Particle &live, Frame *table) {
        ++displayed;
        assert(live.storage == work->particles && live.byte_offset == 96 && table == frames.data());
        assert(live.scalar(Field::PositionX) == 13);
    };
    assert(scheduler.register_callback_context(display_task, binding));
    display_task(scheduler, task);
    assert(displayed == 1);
    task->exec = update_custom_task; work->update = keep_particle;
    update_custom_task(scheduler, task);
    assert(displayed == 2 && particle.scalar(Field::PositionY) == -14);
    work->update = remove_particle; work->active_count = 4;
    update_custom_task(scheduler, task);
    assert(work->active_count == 0 && displayed == 2);
    const auto active = work->active_indices;
    work->active_indices.reset(); work->active_count = 1;
    stopped = false;
    try { update_task(scheduler, task); }
    catch (const Failure &failure) { stopped = failure.operation == Operation::ActiveIndexRead; }
    assert(stopped && work->active_count == 1);
    work->active_indices = active; work->active_count = 0;
    assert(scheduler.destroy_task(task));
    std::cout << "particle lifecycle: scheduler, pending replacement, borrowed slots, cleanup and explicit rendering boundary pass\n";
}
