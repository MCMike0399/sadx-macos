#include "player/gamma_effects.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>

namespace {
using namespace sadx;
std::uint32_t word(const std::vector<std::uint8_t> &bytes, std::size_t at) {
    return std::uint32_t(bytes.at(at)) | (std::uint32_t(bytes.at(at + 1)) << 8) |
           (std::uint32_t(bytes.at(at + 2)) << 16) | (std::uint32_t(bytes.at(at + 3)) << 24);
}
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
void frame_inverse(const particles::Frame &frame, const std::vector<std::uint8_t> &bytes,
                   std::size_t offset) {
    assert((std::uint32_t(std::uint16_t(frame.texture_index)) |
            (std::uint32_t(frame.flags) << 16) | (std::uint32_t(frame.padding) << 24)) == word(bytes, offset));
    assert(static_cast<std::uint32_t>(frame.global_index) == word(bytes, offset + 4));
    const std::array<float, 9> values{frame.width, frame.height, frame.center_x, frame.center_y,
        frame.parameters[0], frame.parameters[1], frame.parameters[2], frame.parameters[3], frame.parameters[4]};
    for (unsigned i = 0; i < values.size(); ++i) assert(bits(values[i]) == word(bytes, offset + 8 + i * 4));
}
}
int main(int argc, char **argv) {
    using namespace sadx;
    assert(argc == 2);
    std::ifstream stream(argv[1], std::ios::binary);
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(stream), {}};
    texture_lifetime::List textures;
    textures.entries.resize(17);
    texture_lifetime::Memory first, second;
    first.global_index = 117; second.global_index = -231;
    first.width = 63; first.height = 37;
    textures.entries[0] = &first;
    textures.entries[1] = &second;
    auto asset = gamma_effects::decode(bytes, textures);
    for (unsigned i = 0; i < asset.hit.size(); ++i) frame_inverse(asset.hit[i], bytes, 20 + i * 44);
    for (unsigned i = 0; i < asset.burst.size(); ++i) frame_inverse(asset.burst[i], bytes, 768 + i * 44);
    assert(asset.textures == &textures);
    for (auto offset : {0U, 8U, 12U, 16U, 20U, 21U + 16 * 44, 768U, 769U + 4 * 44}) {
        auto malformed = bytes;
        malformed[offset] ^= 128;
        bool rejected = false;
        try { (void)gamma_effects::decode(malformed, textures); }
        catch (const std::runtime_error &) { rejected = true; }
        assert(rejected);
    }
    for (auto size : {0U, 19U, 987U, 989U}) {
        auto malformed = bytes;
        malformed.resize(size);
        bool rejected = false;
        try { (void)gamma_effects::decode(malformed, textures); }
        catch (const std::runtime_error &) { rejected = true; }
        assert(rejected);
    }
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    assert(scheduler.initialize_source_pool(arena, {}));
    fidelity::Context arithmetic(0x7f);
    gamma_effects::State state;
    gamma_effects::initialize(state, asset, scheduler, *arena, arithmetic);
    const auto task = state.burst_task;
    assert(task && scheduler.list_head(4) == task && task->primary_work);
    assert(task->primary_work->requested_size() == 64 && task->auxiliary_work->requested_size() == 32);
    assert(task->exec == particles::update_task && task->display == particles::display_task &&
           task->destructor == particles::destroy_task);
    const auto work = std::static_pointer_cast<particles::Work>(task->auxiliary_native_work);
    assert(work->allocation == task->auxiliary_work && work->frames == asset.burst.data());
    assert(work->active_count == 0 && work->capacity == 40 && !work->update &&
           bits(work->parameters.drag) == 0x3cf5c28f && bits(work->parameters.vertical_acceleration) == 0);
    assert(work->free_indices->requested_size() == 80 && work->active_indices->requested_size() == 80 &&
           work->particles->requested_size() == 1920);
    for (unsigned i = 0; i < 40; ++i) {
        std::uint16_t index = 65535;
        assert(arena->read(work->free_indices, i * 2, &index, 2));
        assert(index == i);
    }
    const auto active_begin = work->active_indices->data_offset();
    const auto particle_begin = work->particles->data_offset();
    // InitDLinkHeap supplies 0xff storage; the constructor must leave these bytes intact.
    assert(arena->source_bytes()[active_begin] == 255 && arena->source_bytes()[particle_begin] == 255);
    assert(asset.hit[0].global_index == -231 && asset.hit[0].center_x == 12 &&
           asset.burst[0].global_index == 117 && asset.burst[0].center_x == 0.6f);
    const auto blocks = arena->blocks().size();
    first.global_index = 579; second.global_index = 981;
    gamma_effects::initialize(state, asset, scheduler, *arena, arithmetic);
    assert(state.burst_task == task && asset.hit[0].global_index == 981 &&
           asset.burst[0].global_index == 117 && arena->blocks().size() == blocks);
    for (auto callback : {task->exec, task->display, task->destructor}) {
        bool stopped = false;
        try { callback(scheduler, task); } catch (const std::runtime_error &) { stopped = true; }
        assert(stopped);
    }
    TaskScheduler rollback;
    auto rollback_arena = std::make_shared<TaskWorkArena>();
    assert(rollback.initialize_source_pool(rollback_arena, {}));
    const auto free = *rollback_arena->free_head();
    std::uint32_t available = 0;
    for (const auto &block : rollback_arena->blocks())
        if (block.offset == free) available = *block.physical_next - free;
    assert(available > 128);
    const auto occupied = rollback_arena->allocate(available - 128);
    assert(occupied);
    const auto slots = rollback.source_free_slots();
    assert(!particles::create_pool(rollback, *rollback_arena, 40, {0.125f, -0.5f},
                                  asset.burst.data(), nullptr));
    assert(rollback.source_free_slots() == slots && !rollback.list_head(4));
    // Exhausting task slots is separately observable and does not manufacture a pool.
    TaskScheduler exhausted;
    auto other_arena = std::make_shared<TaskWorkArena>();
    assert(exhausted.initialize_source_pool(other_arena, {}));
    while (exhausted.source_free_slots()) assert(exhausted.allocate_task(3, nullptr));
    assert(!particles::create_pool(exhausted, *other_arena, 40, {0.125f, -0.5f},
                                   asset.burst.data(), nullptr));
    std::cout << "Imported frame inverse, canonical arena/task ownership, cache/reentry and explicit callbacks pass\n";
}
