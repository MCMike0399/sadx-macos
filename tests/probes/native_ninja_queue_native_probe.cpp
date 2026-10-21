#include "engine/ninja_queue_native.hpp"
#include "engine/ninja_sprite_queue.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
namespace {
float value(std::uint32_t bits) {
    float result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}
} // namespace
extern "C" int sadx_native_ninja_queue_lifecycle(const std::uint8_t *tables, std::size_t table_size,
                                                 const std::uint32_t *in, std::size_t count,
                                                 std::uint32_t *out, std::size_t capacity) {
    if (!tables || table_size != 40 || !in || count != 24 || !out || capacity < 80)
        return 0;
    std::array<std::uint32_t, 8> depth;
    std::memcpy(depth.data(), tables, sizeof(depth));
    sadx::queue::Storage storage;
    sadx::queue::State state;
    auto &values = state.values;
    values.depth_comparison = static_cast<std::int32_t>(in[6]);
    values.execution_mode = static_cast<std::int32_t>(in[15]);
    values.lighting_palette = static_cast<std::int32_t>(in[13]);
    values.lighting_copy = static_cast<std::uint8_t>(in[14]);
    values.primary_count = static_cast<std::int32_t>(in[17]);
    values.secondary_count = static_cast<std::int32_t>(in[18]);
    for (unsigned n = 0; n != 3; ++n) {
        values.reset_source[n] = value(in[7 + n]);
        values.allocation_source[n] = value(in[10 + n]);
        values.reset_capture[n] = values.allocation_capture[n] = value(0xefefefef);
    }
    std::array<float, 4> initial, projection;
    for (unsigned n = 0; n != 4; ++n) {
        initial[n] = value(in[2 + n]);
        state.current_material[n] = value(in[19 + n]);
        projection[n] = value(0xdeadbeef);
    }
    auto loop_count = static_cast<std::int32_t>(in[16]);
    sadx::queue::Owners owners{storage, state, initial, loop_count};
    std::vector<std::array<std::uint32_t, 3>> trace;
    const auto issue = [&](std::uint32_t site, std::uint32_t setting, std::uint32_t value) {
        trace.push_back({site, setting, value});
        return in[23] == 0 || trace.size() != in[23];
    };
    sadx::queue::Services services{[&](std::uint32_t mode) {
                                       return issue(0x77ed19, 23, depth.at(mode));
                                   },
                                   [&](bool enabled) {
                                       return issue(0x77ed2f, 14, enabled);
                                   },
                                   [&](bool enabled) {
                                       return issue(0x77ecef, 27, enabled);
                                   },
                                   [&](const std::array<float, 4> &color) {
                                       projection = color;
                                   },
                                   {}};
    std::fill(out, out + capacity, 0);
    sadx::X87Context fp(static_cast<std::uint16_t>(in[1]));
    try {
        sadx::queue::initialize(owners, services, fp, [&] {
            auto arena = std::make_shared<sadx::NinjaRenderQueueArena>(
                std::vector<std::uint8_t>(sadx::queue::kBufferBytes, 205), 0,
                sadx::queue::kBufferBytes, 1);
            return sadx::queue::Buffer{std::move(arena), in[0] & 31};
        });
        out[2] = 1;
    } catch (const sadx::queue::Unavailable &) {
        out[0] = 2;
        out[1] = trace.empty() ? 0 : trace.back()[0];
    }
    const auto &arena = *storage.buffer().arena;
    out[3] = arena.cursor();
    out[4] = std::all_of(arena.bytes().begin(), arena.bytes().end(), [](auto v) {
        return v == 0;
    });
    out[5] = arena.overflow_reported().value_or(255);
    for (unsigned n = 0; n != 4; ++n) {
        out[6 + n] = bits(state.current_material[n]);
        out[10 + n] = bits(projection[n]);
    }
    for (unsigned n = 0; n != 3; ++n) {
        out[14 + n] = bits(values.reset_capture[n]);
        out[17 + n] = bits(values.allocation_capture[n]);
    }
    out[20] = static_cast<std::uint32_t>(values.lighting_palette);
    out[21] = values.lighting_copy;
    out[22] = static_cast<std::uint32_t>(values.execution_mode);
    out[23] = static_cast<std::uint32_t>(loop_count);
    out[24] = static_cast<std::uint32_t>(values.primary_count);
    out[25] = static_cast<std::uint32_t>(values.secondary_count);
    bool empty_buckets = true;
    for (std::size_t bucket = 0; bucket != 2048; ++bucket) {
        const auto front = storage.records().bucket_front(bucket);
        empty_buckets = empty_buckets && front && !front.record;
    }
    out[26] = empty_buckets;
    out[27] = static_cast<std::uint32_t>(trace.size());
    for (std::size_t n = 0; n != trace.size() && n < 8; ++n)
        std::copy(trace[n].begin(), trace[n].end(), out + 28 + 3 * n);
    out[52] = fp.exception_flags() & 63;
    return 1;
}
extern "C" int sadx_native_queue_blend(unsigned packed, unsigned selector, unsigned mode,
                                       unsigned unavailable, unsigned *out) {
    sadx::queue::State state;
    state.values.packed_blend = static_cast<std::uint8_t>(packed);
    sadx::queue::Services services{};
    services.set_blend = [&](unsigned side, unsigned value) {
        out[1] = side;
        out[2] = value;
        out[3] = state.values.packed_blend;
        return unavailable == 0;
    };
    int completed = 1;
    try {
        sadx::queue::set_blend(selector, mode, state, services);
    } catch (const sadx::queue::Unavailable &) {
        completed = 0;
    }
    out[0] = state.values.packed_blend;
    return completed;
}
