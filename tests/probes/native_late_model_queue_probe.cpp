#include "engine/late_model_queue.hpp"

#include <array>
#include <cstring>
#include <memory>
#include <vector>

namespace {
float float_at(std::uint32_t word) {
    float value;
    std::memcpy(&value, &word, sizeof(value));
    return value;
}
std::uint32_t bits_of(float value) {
    std::uint32_t word;
    std::memcpy(&word, &value, sizeof(word));
    return word;
}
constexpr unsigned kModel = 2;
constexpr unsigned kMatrix = 5;
constexpr unsigned kMatrixWords = 16;
constexpr unsigned kOutModel = 4;
constexpr unsigned kOutMatrix = 8;
} // namespace

extern "C" int sadx_native_late_model_queue(const std::uint32_t *input, std::uint32_t *output) {
    if (!input || !output)
        return 1;
    try {
        auto arena = std::make_shared<sadx::NinjaRenderQueueArena>(std::vector<std::uint8_t>(65536, 0xcd), 0,
                                                                    65536, 1);
        sadx::NinjaSpriteQueue queue(arena);
        queue.initialize_sort_buckets();
        sadx::NinjaRenderState render;
        render.packed_blend_at_3ab9890 = 0xa8;
        sadx::NinjaSpriteQueueRuntime runtime;
        runtime.render_state = &render;
        runtime.mode_at_3ab98ac = input[0];
        runtime.flag_at_3abdcfe = 3;
        runtime.captured_word_at_3abd950 = 0x12345678;
        runtime.captured_word_at_3d0f9c8 = 11;
        runtime.captured_word_at_3d0f840 = 22;
        runtime.captured_word_at_3d0f9c4 = 33;
        runtime.color_at_3abd9a8 = std::array<float, 4>{1, 2, 3, 4};
        runtime.color_at_3ab9864 = std::array<float, 4>{5, 6, 7, 8};
        runtime.flag_at_3b12208 = 2;
        runtime.sort_bias_at_3abd9c0 = float_at(input[4]);
        runtime.bucket_bias_at_7dcb60 = 2048.0f;
        sadx::NinjaMatrix matrix{};
        for (unsigned index = 0; index < kMatrixWords; ++index)
            matrix[index] = float_at(input[kMatrix + index]);
        sadx::X87Context arithmetic(static_cast<std::uint16_t>(input[21]));
        const auto result = sadx::late_draw_model(queue, matrix, input[kModel],
                                                   static_cast<std::uint8_t>(input[1]), float_at(input[3]), {},
                                                   runtime, arithmetic);
        if (!result)
            return 2;
        std::memset(output, 0, 24 * sizeof(std::uint32_t));
        output[0] = result.route == sadx::LateModelRoute::queued ? 0 : 1;
        output[1] = result.record ? 1 : 0;
        output[2] = result.bucket ? static_cast<std::uint32_t>(*result.bucket) : 0xffffffffu;
        if (!result.record)
            return 0;
        auto read = [&](std::uint32_t offset, void *data, std::uint32_t size) {
            return arena->read(result.record->block, offset, data, size) == sadx::NinjaQueueFailure::none;
        };
        if (result.route == sadx::LateModelRoute::queued) {
            float depth = 0;
            if (!read(4, &depth, sizeof(depth)))
                return 3;
            output[3] = bits_of(depth);
        }
        std::uint32_t model = 0;
        std::uint8_t header = 0;
        std::uint16_t attribute = 0;
        float color = 0;
        if (!read(0x2c, &model, sizeof(model)) || !read(8, &header, sizeof(header)) ||
            !read(10, &attribute, sizeof(attribute)) || !read(16, &color, sizeof(color)))
            return 3;
        output[kOutModel] = model;
        output[5] = header;
        output[6] = attribute;
        output[7] = bits_of(color);
        for (unsigned index = 0; index < kMatrixWords; ++index) {
            float value = 0;
            if (!read(0x30 + index * 4, &value, sizeof(value)))
                return 3;
            output[kOutMatrix + index] = bits_of(value);
        }
        return 0;
    } catch (const sadx::X87Error &) {
        return 4;
    }
}
