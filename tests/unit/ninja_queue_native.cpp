#include "engine/ninja_queue_native.hpp"
#include "engine/graphics_frame.hpp"
#include "engine/ninja_sprite_queue.hpp"
#include <cassert>
#include <cstring>
#include <vector>
using namespace sadx;
using namespace sadx::queue;
int main() {
    Storage storage;
    State state;
    std::array<float, 4> source{2, -2, .5f, -.5f}, geometry{};
    std::int32_t loop_count = 9;
    Owners owners{storage, state, source, loop_count};
    std::vector<int> calls;
    Services services{[&](std::uint32_t comparison) {
                          calls.push_back(static_cast<int>(comparison));
                          return true;
                      },
                      [&](bool value) {
                          assert(value);
                          calls.push_back(10);
                          return true;
                      },
                      [&](bool value) {
                          assert(value);
                          calls.push_back(11);
                          return true;
                      },
                      [&](const std::array<float, 4> &value) {
                          geometry = value;
                          calls.push_back(12);
                      },
                      {}};
    X87Context fp(127);
    bool missing = false;
    try {
        reset(owners, services, fp);
    } catch (const Unavailable &error) {
        missing = error.boundary() == Boundary::Buffer;
    }
    assert(missing && loop_count == 9 && calls.empty());
    state.values.primary_count = 3;
    state.values.secondary_count = 7;
    state.values.primary_high_water = 17;
    state.values.secondary_high_water = 19;
    state.values.depth_comparison = -1;
    initialize(owners, services, fp);
    assert((calls == std::vector<int>{12, 7, 10, 11, 12}));
    assert((state.current_material == std::array<float, 4>{1, -1, .5f, -.5f}));
    assert((source == std::array<float, 4>{2, -2, .5f, -.5f}));
    assert(geometry == state.current_material);
    assert(loop_count == 0 && state.values.primary_count == 0 && state.values.secondary_count == 0);
    assert(state.values.primary_high_water == 17 && state.values.secondary_high_water == 19);
    assert(state.values.reset_capture == state.values.reset_source);
    assert(state.values.allocation_capture == state.values.allocation_source);
    const auto &arena = storage.buffer().arena;
    const auto remainder = reinterpret_cast<std::uintptr_t>(arena->bytes().data()) % 32;
    assert(storage.buffer().alignment_remainder == remainder);
    assert(arena->cursor() == (32 - remainder) % 32);
    const auto allocated = arena->allocate(16, {});
    assert(allocated && allocated.block);
    std::array<unsigned, 4> payload{1, 2, 3, 4}, readback{};
    assert(arena->write(*allocated.block, 0, payload.data(), sizeof(payload)) ==
           NinjaQueueFailure::none);
    assert(arena->read(*allocated.block, 0, readback.data(), sizeof(readback)) ==
           NinjaQueueFailure::none);
    assert(payload == readback);
    auto *identity = arena.get();
    reset(owners, services, fp);
    assert(arena.get() == identity);
    assert(arena->read(*allocated.block, 0, readback.data(), sizeof(readback)) ==
           NinjaQueueFailure::foreign_record);
    for (std::size_t bucket = 0; bucket != 2048; ++bucket) {
        const auto front = storage.records().bucket_front(bucket);
        assert(front && !front.record);
    }
    std::weak_ptr<NinjaRenderQueueArena> old = arena;
    initialize(owners, services, fp);
    assert(old.expired());
    BlendTables tables{{19, 20}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {13, 14, 15, 16}};
    graphics::Draw draw;
    set_device_blend(99, 0, tables, draw);
    assert(draw.render_states.at(19) == 6 && draw.render_states.at(20) == 5);
    set_device_blend(99, 1, tables, draw);
    assert(draw.render_states.at(19) == 5 && draw.render_states.at(20) == 6);
    set_device_blend(0, 15, tables, draw);
    assert(draw.render_states.at(19) == 16);
    bool selector_rejected = false, value_rejected = false;
    try {
        set_device_blend(2, 8, tables, draw);
    } catch (const Unavailable &error) {
        selector_rejected = error.boundary() == Boundary::BlendSelector;
    }
    try {
        set_device_blend(0, 16, tables, draw);
    } catch (const Unavailable &error) {
        value_rejected = error.boundary() == Boundary::BlendValue;
    }
    assert(selector_rejected && value_rejected);
    state.values.packed_blend = 173;
    services.set_blend = [&](unsigned selector, unsigned mode) {
        assert(selector == 0 && mode == 8 && state.values.packed_blend == 168);
        return false;
    };
    bool unavailable = false;
    try {
        set_blend(0, 8, state, services);
    } catch (const Unavailable &error) {
        unavailable = error.boundary() == Boundary::Blend;
    }
    assert(unavailable && state.values.packed_blend == 168);
    state.current_material = {9, 8, 7, 6};
    std::uint32_t signaling_nan = 2141192193U;
    std::memcpy(&source[0], &signaling_nan, sizeof(signaling_nan));
    X87Context unmasked(126);
    bool trapped = false;
    try {
        reset_material(owners, services, unmasked);
    } catch (const X87Error &error) {
        trapped = error.failure() == X87Failure::unmasked_exception;
    }
    assert(trapped && state.current_material[1] == 8 && state.current_material[2] == 7);
}
