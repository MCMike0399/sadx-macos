#include "engine/particle_pool.hpp"
#include <cstring>
#include <vector>

namespace {
using namespace sadx;
using namespace sadx::particles;
const std::uint32_t *input;
TaskHandle task;
std::shared_ptr<Work> initial, replacement;
std::vector<std::uint32_t> events;
unsigned calls;
std::array<TaskWorkHandle, 3> buffers;
std::uint32_t bits(float f) { std::uint32_t v; std::memcpy(&v, &f, 4); return v; }
float scalar(std::uint32_t v) { float f; std::memcpy(&f, &v, 4); return f; }
std::uint32_t particle_base() {
    return input[6] ? 0x23003000U - (input[6] - 1) : 0x23002000U;
}
std::uint32_t frame_tag(Frame *f) {
    return f ? 0x24000000 + std::uint32_t(f - initial->frames) * 44 : 0;
}
void event(unsigned kind, unsigned identity, Frame *frames = nullptr) {
    events.insert(events.end(), {kind, identity, frame_tag(frames),
        initial->active_count, replacement->active_count});
}
void reenter() {
    ++calls;
    if (calls != 1) return;
    if (input[5] & 1) initial->active_count = 0;
    if (input[5] & 2) {
        initial->active_count = 0;
        task->auxiliary_native_work = replacement;
    }
    if (input[5] & 4) std::swap(initial->free_indices, initial->active_indices);
}
std::int32_t custom(Particle &p, const Parameters &parameters, Frame *frames) {
    event(1, (p.storage ? particle_base() : 0U) + p.byte_offset, frames);
    events.push_back(bits(parameters.drag)); events.push_back(bits(parameters.vertical_acceleration));
    reenter();
    return (input[4] & (1U << (p.byte_offset / 48))) ? 0 : 7;
}
}
namespace sadx::particles {
void display_particle(TaskScheduler &, Particle &p, Frame *frames) {
    event(2, (p.storage ? particle_base() : 0U) + p.byte_offset, frames);
    reenter();
}
void release_buffer(TaskWorkArena &, const TaskWorkHandle &buffer) {
    const auto id = !buffer ? 0 : buffer == buffers[0] ? 1 :
                    buffer == buffers[1] ? 2 : buffer == buffers[2] ? 3 : 4;
    event(3, id);
    reenter();
}
}
extern "C" int particle_lifecycle_case(const std::uint32_t *values, std::uint32_t *output) {
    static_assert(sizeof(Frame) == 44);
    input = values; events.clear(); calls = 0;
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    if (!scheduler.initialize_source_pool(arena, {})) return -1;
    task = scheduler.allocate_task(4, update_task).task;
    initial = std::make_shared<Work>(); replacement = std::make_shared<Work>();
    task->auxiliary_native_work = initial;
    initial->active_count = std::uint16_t(input[2]);
    initial->capacity = std::uint16_t(input[2] >> 16);
    const auto free = arena->allocate(32).work;
    const auto active = arena->allocate(32).work;
    const auto particles = arena->allocate(192).work;
    if (!arena->write(free, 0, input + 16, 32) || !arena->write(active, 0, input + 24, 32) ||
        !arena->write(particles, 0, input + 32, 192)) return -2;
    buffers = {free, active, particles};
    initial->free_indices = input[3] & 1 ? TaskWorkHandle{} : free;
    initial->active_indices = input[3] & 2 ? TaskWorkHandle{} : active;
    initial->particles = input[3] & 4 ? TaskWorkHandle{} : particles;
    std::array<Frame, 4> frames;
    std::memcpy(frames.data(), input + 80, sizeof(frames));
    initial->frames = input[3] & 8 ? nullptr : frames.data();
    initial->parameters = {scalar(input[8]), scalar(input[9])};
    initial->update = input[3] & 16 ? nullptr : custom;
    *replacement = *initial; replacement->active_count = 3;
    std::vector<std::uint8_t> transported;
    if (input[6]) {
        transported = arena->source_bytes();
        const auto failed_offset = particles->data_offset() + input[6] - 1;
        arena->set_payload_transport(
            [&](std::uint32_t at, void *out, std::uint32_t bytes) {
                std::memcpy(out, transported.data() + at, bytes); return true;
            },
            [&, failed_offset](std::uint32_t at, const void *in, std::uint32_t bytes) {
                if (at == failed_offset) return false;
                std::memcpy(transported.data() + at, in, bytes); return true;
            });
    }
    if (input[3] & 32) task->auxiliary_native_work.reset();
    fidelity::Context fp{std::uint16_t(input[1])};
    auto binding = std::make_shared<Binding>(fp);
    if (!scheduler.register_callback_context(update_task, binding)) return -3;
    std::uint32_t boundary = 0, field = 0, result = 0;
    try {
        switch(input[0]) {
        case 0: update_task(scheduler, task); break;
        case 1: update_custom_task(scheduler, task); break;
        case 2: display_task(scheduler, task); break;
        case 3: destroy_task(scheduler, task); break;
        case 4: {
            Particle p{*arena, particles, 0};
            result = std::uint32_t(update_animation(p, initial->frames, fp)); break;
        }
        }
    } catch (const Failure &f) {
        boundary = std::uint32_t(f.operation) + 1; field = std::uint32_t(f.field);
    }
    std::vector<std::uint32_t> data{boundary, field, result, initial->active_count,
        replacement->active_count, task->auxiliary_native_work == replacement ? 1U : 0U};
    std::array<std::uint32_t, 64> payload;
    if (!arena->read(free, 0, payload.data(), 32) || !arena->read(active, 0, payload.data() + 8, 32) ||
        !arena->read(particles, 0, payload.data() + 16, 192)) return -4;
    data.insert(data.end(), payload.begin(), payload.end());
    const auto *frame_words = reinterpret_cast<const std::uint32_t *>(frames.data());
    data.insert(data.end(), frame_words, frame_words + 44);
    data.push_back(events.size()); data.insert(data.end(), events.begin(), events.end());
    data.push_back(fp.exception_flags() & 63);
    std::memcpy(output, data.data(), data.size() * 4);
    return int(data.size());
}
