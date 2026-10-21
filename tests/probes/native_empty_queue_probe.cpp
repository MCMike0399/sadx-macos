#include "engine/queue_consumer_native.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
namespace {
float number(std::uint32_t value) {
    float result;
    std::memcpy(&result, &value, 4);
    return result;
}
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
struct Stop {};
} // namespace
extern "C" int sadx_native_empty_queue(const std::uint32_t *input, std::uint32_t *output) {
    using namespace sadx;
    static auto buffer = queue::allocate_buffer();
    buffer.arena->reset_407ab0(0);
    queue::Storage storage;
    storage.bind_buffer(buffer);
    storage.records().initialize_sort_buckets();
    queue::State state;
    auto &v = state.values;
    v.execution_mode = static_cast<std::int32_t>(input[2]);
    v.primary_count = static_cast<std::int32_t>(input[3]);
    v.primary_high_water = static_cast<std::int32_t>(input[4]);
    v.secondary_count = static_cast<std::int32_t>(input[5]);
    v.secondary_high_water = static_cast<std::int32_t>(input[6]);
    v.lighting_palette = static_cast<std::int32_t>(input[7]);
    v.lighting_copy = static_cast<std::uint8_t>(input[8]);
    std::int32_t disabled = static_cast<std::int32_t>(input[1]);
    std::array<float, 4> initial;
    for (unsigned i = 0; i < 4; ++i) {
        initial[i] = number(input[16 + i]);
        state.current_material[i] = number(input[20 + i]);
    }
    std::array<float, 4> device_color{};
    NinjaMatrix matrix;
    for (unsigned i = 0; i < 16; ++i)
        matrix[i] = number(input[24 + i]);
    std::uint8_t special = static_cast<std::uint8_t>(input[9]),
                 fog = static_cast<std::uint8_t>(input[10]);
    std::uint32_t control = input[11], mask = input[12], attributes = input[13];
    texture_lifetime::List list;
    const texture_lifetime::List *queued = input[40] ? &list : nullptr;
    queue::Owners owners{storage, state, initial, disabled};
    X87Context fp(static_cast<std::uint16_t>(input[0]));
    if (input[14]) {
        NinjaRenderState render;
        render.packed_blend_at_3ab9890 = 0;
        NinjaSpriteQueueRuntime runtime;
        runtime.mode_at_3ab98ac = 0;
        runtime.flag_at_3abdcfe = 0;
        runtime.render_state = &render;
        runtime.captured_word_at_3abd950 = 0;
        runtime.captured_texture_list_at_3abd950 = std::shared_ptr<NinjaTextureListOwner>{};
        runtime.captured_word_at_3d0f9c8 = 0;
        runtime.captured_word_at_3d0f840 = 0;
        runtime.captured_word_at_3d0f9c4 = 0;
        runtime.color_at_3ab9864 = std::array<float, 4>{};
        runtime.flag_at_3b12208 = 0;
        runtime.sort_bias_at_3abd9c0 = 0;
        runtime.bucket_bias_at_7dcb60 = 0;
        auto allocation = storage.records().allocate_record(0x98, 2, 0, 0, runtime);
        if (!allocation || !allocation.record)
            return 0;
        auto inserted = storage.records().insert(allocation.record, 12048, 1, runtime, fp);
        if (!inserted)
            return 0;
        fp.clear_exceptions();
    }
    std::vector<std::array<std::uint32_t, 6>> trace;
    const auto event = [&](std::uint32_t id, std::uint32_t arg,
                           const std::array<float, 4> *color = nullptr) {
        std::array<std::uint32_t, 6> row{id, arg, 0, 0, 0, 0};
        if (color)
            for (unsigned i = 0; i < 4; ++i)
                row[i + 2] = bits((*color)[i]);
        trace.push_back(row);
        if (trace.size() == input[15])
            throw Stop{};
        if (trace.size() == 1 && input[41]) {
            control = input[42];
            mask = input[43];
            attributes = input[44];
            for (unsigned i = 0; i < 4; ++i)
                state.current_material[i] = number(input[45 + i]);
        }
    };
    queue::Services services{[&](std::uint32_t mode) {
                                 event(4, mode);
                                 return true;
                             },
                             [&](bool write) {
                                 event(5, write);
                                 return true;
                             },
                             [](bool) { return true; },
                             [&](const std::array<float, 4> &color) {
                                 event(1, 0, &color);
                                 device_color = color;
                             },
                             [](std::uint32_t, std::uint32_t) { return true; }};
    queue::ConsumerServices calls{services,
                                  [&] {
                                      event(2, 0);
                                      return matrix;
                                  },
                                  [&](const NinjaMatrix &value) {
                                      event(3, 0);
                                      matrix = value;
                                  },
                                  [&](bool value) { event(6, value); },
                                  {}};
    queue::ConsumerResult result;
    std::uint32_t status = 0;
    try {
        result =
            queue::consume({owners, control, mask, attributes, special, fog, queued}, calls, fp);
        status = static_cast<std::uint32_t>(result.boundary);
    } catch (const Stop &) {
        status = 100;
    }
    std::fill_n(output, 256, 0);
    const std::uint32_t words[] = {status,
                                   result.completed,
                                   static_cast<std::uint32_t>(v.execution_mode),
                                   static_cast<std::uint32_t>(v.primary_high_water),
                                   static_cast<std::uint32_t>(v.secondary_high_water),
                                   static_cast<std::uint32_t>(v.lighting_palette),
                                   v.lighting_copy,
                                   control,
                                   mask,
                                   attributes,
                                   queued != nullptr,
                                   fog,
                                   special,
                                   static_cast<std::uint32_t>(trace.size()),
                                   static_cast<std::uint32_t>(fp.exception_flags() & 63),
                                   0};
    std::copy_n(words, 16, output);
    for (unsigned i = 0; i < 4; ++i) {
        output[16 + i] = bits(state.current_material[i]);
        output[20 + i] = bits(device_color[i]);
    }
    for (unsigned i = 0; i < 16; ++i)
        output[24 + i] = bits(matrix[i]);
    output[40] = result.record != nullptr;
    output[41] = static_cast<std::uint32_t>(result.bucket);
    for (std::size_t i = 0; i < trace.size(); ++i)
        std::copy(trace[i].begin(), trace[i].end(), output + 48 + i * 6);
    return 1;
}
