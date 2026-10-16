#include "runtime/engine_code_tables.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>

// Serialization belongs to the reference probe, not the native table representation.
extern "C" std::size_t sadx_native_engine_code_tables(std::uint32_t *output, std::size_t capacity) {
    if (!output || capacity < 287)
        return 0;
    const auto tables = sadx::runtime::engine_code_tables();
    std::size_t count = 0;
    const auto word = [&](auto value) { output[count++] = static_cast<std::uint32_t>(value); };
    const auto real = [&](float value) {
        std::uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        word(bits);
    };
    for (const auto value : tables.blend_targets)
        word(value);
    for (const auto value : tables.blend_modes)
        word(value);
    for (const auto value : tables.depth_comparisons)
        word(value);
    for (const auto value : tables.keyboard)
        word(static_cast<std::uint32_t>(value.kind) | (std::uint32_t(value.code) << 16));
    for (const auto value : tables.unit_matrix)
        real(value);
    real(tables.sprite.uv_scale);
    real(tables.sprite.color_scale);
    real(tables.sprite.minimum_depth);
    real(tables.sprite.zero);
    real(tables.sprite.one);
    real(tables.negative_one);
    real(tables.fog_disabled);
    real(tables.view.width_scale);
    real(tables.view.height_scale);
    real(tables.view.half);
    real(tables.view.horizontal_extent);
    real(tables.view.vertical_extent);
    return count;
}
