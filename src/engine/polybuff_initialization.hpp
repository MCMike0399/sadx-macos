#pragma once
#include "engine/normal_uv_strip.hpp"
#include <array>

namespace sadx::polybuff {
constexpr std::size_t kBufferCount = 9;
constexpr std::size_t kScratchWords = 128;
constexpr std::uint32_t kScratchVertexBytes = 36;
using VertexScratch = BufferMemory *;
using BufferBase = void *;
struct Buffers {
    // GLOBAL: SONIC 0x3d0fe00 PolygonBuffers size=324
    std::array<BufferState, kBufferCount> records = {};
    // GLOBAL: SONIC 0x3d128ac GiantVertexBuffer_ptr size=4
    VertexScratch vertices = nullptr;
};
struct RetainedBase {
    // GLOBAL: SONIC 0x3d127a0 PolygonBufferBase size=4
    BufferBase value = nullptr;
};
struct Scratch {
    // GLOBAL: SONIC 0x3d0fae0 PolygonBufferScratch size=512
    std::array<std::uint32_t, kScratchWords> words = {};
};
// NOTSA: counts borrow the configured library State, after its configuration copy.
// No second count owner or inferred draw capacity is introduced here.
struct ConfiguredCounts {
    const std::array<std::uint32_t, kBufferCount> &values;
};
struct BufferDescription {
    std::uint32_t stride;
    std::uint32_t vertex_format;
    const char *debug_name;
};
enum class InitializationField { RetainedBase, Vertices, ScratchWord };
// NOTSA: typed source table reads keep reference availability outside the game algorithm.
ConfiguredCounts configured_counts(Context &);
std::uint32_t configured_count(Context &, std::size_t index);
BufferDescription buffer_description(Context &, std::size_t index);
BufferMemory *allocate_vertex_scratch(Context &, std::uint32_t bytes);
void observe_initialization_store(Context &, InitializationField, std::size_t index);
// NOTSA: named step binds the recovered per-record helper, or a reference receipt boundary.
void initialize_record(Context &, BufferState &, std::size_t index, std::uint32_t count,
                       BufferDescription);
// NOTSA: maintained literal table, checked byte-for-byte against pinned source data.
const std::array<BufferDescription, kBufferCount> &buffer_descriptions();
// NOTSA: Context is selected at compile time; ADL binds the same algorithm to native
// device functions or reference-only receipt/fact functions, without shared fixture fields.
// FUNCTION: SONIC 0x78e720 InitPolyBuffers
template <class InitContext> void initialize_buffers(
    InitContext &context, Buffers &buffers, RetainedBase &retained, Scratch &scratch,
    std::uint32_t ignored_count,
    std::uint32_t vertex_count, void *base) {
    (void)ignored_count; // Original first parameter is never read.
    observe_initialization_store(context, InitializationField::RetainedBase, 0);
    retained.value = base;
    auto *vertices = allocate_vertex_scratch(context, vertex_count * kScratchVertexBytes);
    observe_initialization_store(context, InitializationField::Vertices, 0);
    buffers.vertices = vertices;
    for (std::size_t index = 0; index != kBufferCount; ++index) {
        // The source reads name, format and stride before the configured count.
        const auto description = buffer_description(context, index);
        const auto count = configured_count(context, index);
        initialize_record(context, buffers.records[index], index, count, description);
    }
    for (std::size_t index = 0; index != kScratchWords; ++index) {
        observe_initialization_store(context, InitializationField::ScratchWord, index);
        scratch.words[index] = 0;
    }
}
} // namespace sadx::polybuff
