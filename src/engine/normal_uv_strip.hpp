#pragma once

#include "cpu/fidelity_float.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace sadx::polybuff {
struct Context;
struct DeviceBuffer;
struct BufferMemory;

// NOTSA: imported byte ranges remain borrowed, including an explicitly supplied negative origin.
enum class DataKind {
    Mesh, TextureCoordinates, Points, Normals, RenderArguments, VertexBytes, DeviceResource
};
struct DataFault : std::runtime_error {
    DataKind kind;
    std::int64_t index;
    DataFault(DataKind kind, std::int64_t index);
};
template <class T> struct BorrowedRange {
    T *data;
    std::size_t size;
    std::int64_t first_index;
    DataKind kind;
    T &at(std::int64_t index) const {
        if (index < first_index || static_cast<std::uint64_t>(index - first_index) >= size)
            throw DataFault(kind, index);
        return data[index - first_index];
    }
};
struct Mesh {
    std::uint16_t strip_count;
    BorrowedRange<const std::int16_t> indices;
    BorrowedRange<const std::int16_t> texture_coordinates;
};
struct Vectors {
    BorrowedRange<const std::uint32_t> components;
};
// NOTSA: device resource ownership and writable CPU bytes are deliberately separate.
// Lock can fail without replacing its output. The stale offset then remains the stride,
// as in the original local variable; the first byte write must report that failure.
struct VertexCursor {
    BufferMemory *memory;
    std::uint32_t byte_offset;
};
constexpr std::uint32_t kRenderArgumentBytes = 16;
constexpr std::uint32_t kWordBytes = 4;
enum class ArgumentField : std::uint32_t { StartVertex, PrimitiveCount, CullMode, Unused };
// NOTSA: named argument fields over bounded source-compatible storage. Byte-offset
// wrap happens before field selection, matching the source row-stride arithmetic.
struct RenderArguments {
    BorrowedRange<std::uint32_t> words;
    std::uint32_t &at(std::uint32_t row, ArgumentField field) const {
        const auto byte_offset = row * kRenderArgumentBytes;
        return words.at(byte_offset / kWordBytes + static_cast<std::uint32_t>(field));
    }
};
enum class BufferField {
    Resource, Capacity, Remaining, Stride, Format, Arguments, LockCount, DebugName, Unused
};
struct BufferState {
    DeviceBuffer *resource;
    std::uint32_t capacity_bytes;
    std::uint32_t remaining_bytes;
    std::uint32_t stride;
    std::uint32_t vertex_format;
    RenderArguments render_arguments;
    std::uint32_t lock_count;
    const char *debug_name;
    std::uint32_t unused;
};
constexpr std::uint32_t kStripLengthMask = 0x3fff;
constexpr std::uint32_t kNoOverwrite = 0x1000;
constexpr std::uint32_t kDiscard = 0x2000;
constexpr std::uint32_t kDynamicWriteOnly = 0x208;
constexpr std::uint32_t kCullModeState = 22;
constexpr std::uint32_t kTriangleStripPrimitive = 5;
constexpr std::uint32_t kNormalUvVertexBytes = 36;
constexpr float kUvScale = 0x1.010102p-8f;

// NOTSA: device and observation seams, bound by the runtime or reference test at link time.
// [platform:d3d8] HRESULTs are intentionally ignored by the recovered callers.
void set_vertex_shader(Context &context, std::uint32_t format);
void set_stream_source(Context &context, DeviceBuffer *resource, std::uint32_t stream,
                       std::uint32_t stride);
void lock_vertices(Context &context, DeviceBuffer *resource, std::uint32_t offset,
                   std::uint32_t bytes, VertexCursor &output, std::uint32_t flags);
std::int32_t unlock_vertices(Context &context, DeviceBuffer *resource);
void set_render_state(Context &context, std::uint32_t state, std::uint32_t value);
void draw_primitive(Context &context, std::uint32_t type, std::uint32_t start, std::uint32_t count);
void create_vertex_buffer(Context &context, BufferState &buffer, std::uint32_t bytes,
                          std::uint32_t usage, std::uint32_t format, std::uint32_t pool);
RenderArguments allocate_arguments(Context &context, std::uint32_t bytes);
void release_vertex_buffer(Context &context, DeviceBuffer *resource);
void free_arguments(Context &context, RenderArguments arguments);
void store_vertex_word(Context &context, VertexCursor output, std::uint32_t word,
                       std::uint32_t value);
void observe_buffer_store(Context &context, const BufferState &buffer, BufferField field,
                          std::uint32_t value);
void observe_argument_store(Context &context, std::uint32_t row, ArgumentField field,
                            std::uint32_t value);
fidelity::ExactFloat exact(Context &context, float value);

// FUNCTION: SONIC 0x794540 PolyBuff_Init
void initialize(Context &context, BufferState *buffer, std::uint32_t count, std::uint32_t stride,
                std::uint32_t format, const char *debug_name);
// FUNCTION: SONIC 0x7945c0 PolyBuff_Free
void release(Context &context, BufferState *buffer);
// FUNCTION: SONIC 0x794600 PolyBuff_SetCurrent
void set_current(Context &context, BufferState &buffer);
// FUNCTION: SONIC 0x794630 PolyBuff_LockTriangleStrip
VertexCursor lock_triangle_strip(Context &context, BufferState &buffer, std::uint32_t vertices,
                                std::uint32_t cull_mode);
// FUNCTION: SONIC 0x794750 PolyBuff_Unlock
std::int32_t unlock(Context &context, BufferState &buffer);
// FUNCTION: SONIC 0x794760 PolyBuff_DrawTriangleStrip
void draw_triangle_strip(Context &context, BufferState &buffer);
// FUNCTION: SONIC 0x794800 PolyBuff_Discard
void discard(Context &context, BufferState &buffer);
// FUNCTION: SONIC 0x7835f0 polybuff_normal_uv_strip
void normal_uv_strip(Context &context, BufferState &buffer, const Mesh &mesh, const Vectors &points,
                     const Vectors &normals, std::uint32_t color, std::uint32_t cull_mode);
} // namespace sadx::polybuff
