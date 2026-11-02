#include "player/gamma_effects.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace {
using namespace sadx;
constexpr std::uint32_t kTask = 0x21000000, kOld = 0x22000000, kMarker = 0x23000000;
constexpr std::uint32_t kBuffer = 0x11000000, kBurst = 0x91d458;
const std::uint32_t *input;
gamma_effects::State *state;
particles::Parameters *live_parameters;
TaskHandle task, old_task, marker_task;
std::array<TaskWorkHandle, 4> allocations;
std::array<std::uint32_t, 4> sizes;
std::vector<std::uint32_t> events;
unsigned allocate_calls, create_calls, destroyed;
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
float scalar(std::uint32_t value) {
    float result;
    std::memcpy(&result, &value, 4);
    return result;
}
std::uint32_t task_tag(const TaskHandle &value) {
    if (!value) return 0;
    if (value == task) return kTask;
    if (value == old_task) return kOld;
    if (value == marker_task) return kMarker;
    throw std::runtime_error("unknown task identity");
}
std::uint32_t buffer_tag(const TaskWorkHandle &value) {
    if (!value) return 0;
    for (unsigned index = 0; index < allocations.size(); ++index)
        if (value == allocations[index]) return kBuffer + index * 4096;
    throw std::runtime_error("unknown buffer identity");
}
std::uint32_t callback_tag(TaskCallback callback) {
    if (!callback) return 0;
    if (callback == particles::update_task) return 0x4cc080;
    if (callback == particles::update_custom_task) return 0x4cc150;
    if (callback == particles::display_task) return 0x4cc040;
    if (callback == particles::destroy_task) return 0x4cc1e0;
    throw std::runtime_error("unknown callback identity");
}
std::int32_t custom_update(particles::Particle &, const particles::Parameters &, particles::Frame *) {
    throw std::runtime_error("constructor invoked custom update");
}
void event(std::uint32_t kind, std::uint32_t first, std::uint32_t second,
           std::uint32_t third) {
    auto work = task ? std::static_pointer_cast<particles::Work>(task->auxiliary_native_work) : nullptr;
    const std::uint32_t row[] = {kind, first, second, third, task_tag(state->burst_task),
        task ? buffer_tag(task->auxiliary_work) : 0,
        work ? std::uint32_t(work->active_count) | (std::uint32_t(work->capacity) << 16) : 0,
        work && allocate_calls >= 3 ? buffer_tag(work->free_indices) : 0,
        work && allocate_calls >= 4 ? buffer_tag(work->active_indices) : 0};
    events.insert(events.end(), std::begin(row), std::end(row));
}
void reenter() {
    if (input[10] & 1) state->burst_task = marker_task;
    if ((input[10] & 2) && input[0] == 2) {
        live_parameters->drag = scalar(input[14]);
        live_parameters->vertical_acceleration = scalar(input[15]);
    }
}
void decode_frame(particles::Frame &frame, const std::uint32_t *words) {
    frame.texture_index = static_cast<std::int16_t>(words[0]);
    frame.flags = static_cast<std::uint8_t>(words[0] >> 16);
    frame.padding = static_cast<std::uint8_t>(words[0] >> 24);
    std::memcpy(&frame.global_index, words + 1, 4);
    frame.width = scalar(words[2]); frame.height = scalar(words[3]);
    frame.center_x = scalar(words[4]); frame.center_y = scalar(words[5]);
    for (unsigned i = 0; i < 5; ++i) frame.parameters[i] = scalar(words[i + 6]);
}
void encode_frame(std::vector<std::uint32_t> &output, const particles::Frame &frame) {
    output.push_back(std::uint16_t(frame.texture_index) | (std::uint32_t(frame.flags) << 16) |
                     (std::uint32_t(frame.padding) << 24));
    output.push_back(static_cast<std::uint32_t>(frame.global_index));
    output.push_back(bits(frame.width)); output.push_back(bits(frame.height));
    output.push_back(bits(frame.center_x)); output.push_back(bits(frame.center_y));
    for (auto value : frame.parameters) output.push_back(bits(value));
}
}
namespace sadx::particles {
TaskHandle create_task(TaskScheduler &scheduler) {
    ++create_calls;
    event(1, 2, 4, 0x4cc080);
    reenter();
    if (input[4]) return {};
    task = scheduler.allocate_task(4, update_task).task;
    return task;
}
TaskWorkHandle allocate_buffer(TaskWorkArena &arena, std::uint32_t bytes) {
    ++allocate_calls;
    if (allocate_calls > 4) throw std::runtime_error("unexpected repeat allocation");
    event(2, bytes, allocate_calls, 0);
    reenter();
    const auto index = allocate_calls - 1;
    sizes[index] = bytes;
    if (input[5] == allocate_calls || !bytes || bytes > 8192) return {};
    const auto result = arena.allocate(bytes);
    if (!result) throw std::runtime_error("probe arena exhausted");
    allocations[index] = result.work;
    if (!arena.fill(result.work, 0, static_cast<std::uint8_t>(0xa1 + index), bytes))
        throw std::runtime_error("probe fill failed");
    return result.work;
}
void discard_task(TaskScheduler &, const TaskHandle &value) {
    if (value != task) throw std::runtime_error("wrong destroyed task");
    ++destroyed;
    event(3, kTask, 0, 0);
    reenter();
}
void load_textures(texture_lifetime::List &list) {
    event(4, 0x91d15c, 0, 0);
    reenter();
    if (input[13]) std::swap(list.entries[0], list.entries[1]);
}
void update_task(TaskScheduler &, const TaskHandle &) { throw std::runtime_error("update"); }
void update_custom_task(TaskScheduler &, const TaskHandle &) { throw std::runtime_error("custom"); }
void display_task(TaskScheduler &, const TaskHandle &) { throw std::runtime_error("display"); }
void destroy_task(TaskScheduler &, const TaskHandle &) { throw std::runtime_error("destroy"); }
}
extern "C" int gamma_effects_case(const std::uint32_t *values, std::uint32_t *output) {
    try {
        input = values;
        task.reset(); old_task.reset(); marker_task.reset();
        allocations = {}; sizes = {}; events.clear();
        allocate_calls = create_calls = destroyed = 0;
        TaskScheduler scheduler;
        auto arena = std::make_shared<TaskWorkArena>();
        if (!scheduler.initialize_source_pool(arena, {})) return -1;
        old_task = scheduler.allocate_task(5, nullptr).task;
        marker_task = scheduler.allocate_task(6, nullptr).task;
        gamma_effects::State local;
        state = &local;
        if (input[3]) local.burst_task = old_task;
        gamma_effects::Asset asset{};
        for (unsigned i = 0; i < 17; ++i) decode_frame(asset.hit[i], input + 16 + i * 11);
        for (unsigned i = 0; i < 5; ++i) decode_frame(asset.burst[i], input + 16 + (i + 17) * 11);
        std::array<texture_lifetime::Memory, 17> memories;
        texture_lifetime::List textures;
        for (unsigned i = 0; i < 17; ++i) {
            const auto offset = 16 + 22 * 11 + i * 3;
            std::memcpy(&memories[i].global_index, input + offset, 4);
            memories[i].width = input[offset + 1];
            memories[i].height = input[offset + 2];
            textures.entries.push_back(input[12] & (1U << i) ? nullptr : &memories[i]);
        }
        asset.textures = &textures;
        particles::Parameters parameters{scalar(input[8]), scalar(input[9])};
        live_parameters = &parameters;
        fidelity::Context arithmetic(static_cast<std::uint16_t>(input[1]));
        TaskHandle result;
        std::uint32_t boundary = 0;
        try {
            for (unsigned repeat = 0; repeat < input[11]; ++repeat) {
                if (input[0] == 0)
                    gamma_effects::initialize(local, asset, scheduler, *arena, arithmetic);
                else if (input[0] == 1)
                    particles::bind_frames(textures, asset.hit.data(),
                        static_cast<std::uint8_t>(input[2]), arithmetic);
                else
                    result = particles::create_pool(scheduler, *arena,
                        static_cast<std::int32_t>(input[6]), parameters, asset.burst.data(),
                        input[7] ? custom_update : nullptr);
            }
        } catch (const std::runtime_error &error) {
            if (std::string(error.what()) != "particle_pool_free_index_store") throw;
            boundary = 0x4cc2ab;
        }
        std::vector<std::uint32_t> row{boundary, task_tag(local.burst_task), task_tag(result),
            allocate_calls, create_calls, destroyed, static_cast<std::uint32_t>(events.size())};
        row.insert(row.end(), events.begin(), events.end());
        row.push_back(task ? callback_tag(task->exec) : 0);
        row.push_back(task ? callback_tag(task->display) : 0);
        row.push_back(task ? callback_tag(task->destructor) : 0);
        row.push_back(task ? buffer_tag(task->auxiliary_work) : 0);
        auto work = task ? std::static_pointer_cast<particles::Work>(task->auxiliary_native_work) : nullptr;
        if (work) {
            row.push_back(std::uint32_t(work->active_count) | (std::uint32_t(work->capacity) << 16));
            row.push_back(buffer_tag(work->free_indices));
            row.push_back(buffer_tag(work->active_indices));
            row.push_back(buffer_tag(work->particles));
            row.push_back(work->update == custom_update ? 0x24000000 : 0);
            row.push_back(bits(work->parameters.drag));
            row.push_back(bits(work->parameters.vertical_acceleration));
            if (work->frames != asset.burst.data()) return -2;
            row.push_back(kBurst);
        } else row.insert(row.end(), 8, 0);
        for (unsigned index = 1; index < 4; ++index) {
            const auto bytes = allocations[index] ? sizes[index] : 0;
            row.push_back(bytes);
            std::vector<std::uint8_t> raw(bytes);
            if (bytes && !arena->read(allocations[index], 0, raw.data(), bytes)) return -3;
            for (unsigned i = 0; i < bytes; ++i) row.push_back(raw[i]);
        }
        for (const auto &frame : asset.hit) encode_frame(row, frame);
        for (const auto &frame : asset.burst) encode_frame(row, frame);
        row.push_back(arithmetic.exception_flags());
        std::copy(row.begin(), row.end(), output);
        return static_cast<int>(row.size());
    } catch (...) { return -4; }
}
