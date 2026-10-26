#include "engine/normal_uv_strip.hpp"
#include <cstring>

namespace sadx::polybuff {
DataFault::DataFault(DataKind kind, std::int64_t index)
    : std::runtime_error("unavailable polybuffer data"), kind(kind), index(index) {}

namespace {
// NOTSA: write a source argument word without initializing its untouched fourth word.
void argument(Context &context, BufferState &buffer, std::uint32_t row, ArgumentField field,
              std::uint32_t value) {
    auto &destination = buffer.render_arguments.at(row, field);
    observe_argument_store(context, row, field, value);
    destination = value;
}
// NOTSA: preserve component-copy and U-then-V arithmetic/store order for each emitted vertex.
void vertex(Context &context, VertexCursor output, const Mesh &mesh, const Vectors &points,
            const Vectors &normals, std::int64_t index_cursor, std::int64_t uv_cursor,
            std::uint32_t color) {
    const auto index = mesh.indices.at(index_cursor); // Original MOVSX, including negative indices.
    const auto first_component = static_cast<std::int64_t>(index) * 3;
    for (std::uint32_t axis = 0; axis != 3; ++axis)
        store_vertex_word(context, output, axis, points.components.at(first_component + axis));
    for (std::uint32_t axis = 0; axis != 3; ++axis)
        store_vertex_word(context, output, axis + 3, normals.components.at(first_component + axis));
    store_vertex_word(context, output, 6, color);
    for (std::uint32_t axis = 0; axis != 2; ++axis) {
        // Signed16 integers convert exactly to binary32; the retained product still uses
        // the caller's precision/exponent/rounding context, which has no entry census here.
        const auto coordinate = mesh.texture_coordinates.at(uv_cursor + axis);
        const auto scaled = (exact(context, static_cast<float>(coordinate)) * kUvScale).rounded();
        std::uint32_t bits;
        std::memcpy(&bits, &scaled, sizeof(bits));
        store_vertex_word(context, output, 7 + axis, bits);
    }
}
} // namespace

void initialize(Context &context, BufferState *buffer, std::uint32_t count, std::uint32_t stride,
                std::uint32_t format, const char *debug_name) {
    if (!buffer)
        return;
    const auto bytes = count * stride;
    observe_buffer_store(context, *buffer, BufferField::Stride, stride);
    buffer->stride = stride;
    observe_buffer_store(context, *buffer, BufferField::Capacity, bytes);
    buffer->capacity_bytes = bytes;
    observe_buffer_store(context, *buffer, BufferField::Remaining, bytes);
    buffer->remaining_bytes = bytes;
    observe_buffer_store(context, *buffer, BufferField::Format, format);
    buffer->vertex_format = format;
    create_vertex_buffer(context, *buffer, bytes, kDynamicWriteOnly, format, 0);
    auto arguments = allocate_arguments(context, count * kRenderArgumentBytes);
    observe_buffer_store(context, *buffer, BufferField::Arguments, arguments.words.data ? 1U : 0U);
    buffer->render_arguments = arguments;
    observe_buffer_store(context, *buffer, BufferField::LockCount, 0);
    buffer->lock_count = 0;
    for (std::uint32_t row = 0; row != count; ++row) {
        argument(context, *buffer, row, ArgumentField::StartVertex, 0);
        argument(context, *buffer, row, ArgumentField::PrimitiveCount, 0);
    }
    observe_buffer_store(context, *buffer, BufferField::DebugName, 0);
    buffer->debug_name = debug_name;
    observe_buffer_store(context, *buffer, BufferField::Unused, 0);
    buffer->unused = 0;
}

void release(Context &context, BufferState *buffer) {
    if (!buffer)
        return;
    if (buffer->resource) {
        release_vertex_buffer(context, buffer->resource);
        observe_buffer_store(context, *buffer, BufferField::Resource, 0);
        buffer->resource = nullptr;
    }
    if (buffer->render_arguments.words.data) {
        free_arguments(context, buffer->render_arguments);
        observe_buffer_store(context, *buffer, BufferField::Arguments, 0);
        buffer->render_arguments = {{nullptr, 0, 0, DataKind::RenderArguments}};
    }
}

void set_current(Context &context, BufferState &buffer) {
    set_vertex_shader(context, buffer.vertex_format);
    set_stream_source(context, buffer.resource, 0, buffer.stride);
}

VertexCursor lock_triangle_strip(Context &context, BufferState &buffer, std::uint32_t vertices,
                                std::uint32_t cull_mode) {
    const auto stride = buffer.stride;
    const auto bytes = stride * vertices; // Original 32-bit product wraps.
    const bool append = bytes < buffer.remaining_bytes; // Equality discards too.
    const auto available = append ? buffer.remaining_bytes : buffer.capacity_bytes;
    const auto offset = buffer.capacity_bytes - available;
    observe_buffer_store(context, buffer, BufferField::Remaining, available - bytes);
    buffer.remaining_bytes = available - bytes;
    if (stride == 0)
        throw std::domain_error("polybuffer zero stride division");
    const auto row = buffer.lock_count;
    argument(context, buffer, row, ArgumentField::PrimitiveCount, vertices - 2);
    argument(context, buffer, row, ArgumentField::StartVertex, offset / stride);
    argument(context, buffer, row, ArgumentField::CullMode, cull_mode);
    observe_buffer_store(context, buffer, BufferField::LockCount, buffer.lock_count + 1);
    ++buffer.lock_count;
    VertexCursor output{nullptr, stride};
    lock_vertices(context, buffer.resource, offset, bytes, output, append ? kNoOverwrite : kDiscard);
    return output;
}

std::int32_t unlock(Context &context, BufferState &buffer) {
    return unlock_vertices(context, buffer.resource);
}

void draw_triangle_strip(Context &context, BufferState &buffer) {
    const auto count = buffer.lock_count;
    // Source captures the argument-array identity before the first device callback.
    const auto arguments = buffer.render_arguments;
    for (std::uint32_t row = 0; row != count; ++row) {
        set_render_state(context, kCullModeState, arguments.at(row, ArgumentField::CullMode));
        const auto primitive_count = arguments.at(row, ArgumentField::PrimitiveCount);
        const auto start = arguments.at(row, ArgumentField::StartVertex);
        draw_primitive(context, kTriangleStripPrimitive, start, primitive_count);
    }
    observe_buffer_store(context, buffer, BufferField::LockCount, 0);
    buffer.lock_count = 0;
}

void discard(Context &context, BufferState &buffer) {
    if (!buffer.resource)
        return;
    observe_buffer_store(context, buffer, BufferField::Remaining,
                         buffer.capacity_bytes - buffer.stride);
    buffer.remaining_bytes = buffer.capacity_bytes - buffer.stride;
    VertexCursor ignored{nullptr, 0};
    lock_vertices(context, buffer.resource, 0, buffer.stride, ignored, kDiscard);
    unlock_vertices(context, buffer.resource);
}

void normal_uv_strip(Context &context, BufferState &buffer, const Mesh &mesh, const Vectors &points,
                     const Vectors &normals, std::uint32_t color, std::uint32_t cull_mode) {
    // The original retains both input pointers across device callbacks, but reloads
    // the strip count after Lock. Borrowed descriptors preserve those identities.
    const auto input = mesh;
    std::uint32_t vertices = 0;
    std::int64_t cursor = 0;
    for (std::uint32_t strip = 0; strip != input.strip_count; ++strip) {
        const auto count = static_cast<std::uint16_t>(input.indices.at(cursor)) & kStripLengthMask;
        vertices += count + 2;
        cursor += count + 1;
    }
    set_current(context, buffer);
    auto output = lock_triangle_strip(context, buffer, vertices, cull_mode);
    cursor = 0;
    std::int64_t uv_cursor = 0;
    const auto strip_count = mesh.strip_count;
    for (std::uint32_t strip = 0; strip != strip_count; ++strip) {
        const auto count = static_cast<std::uint16_t>(input.indices.at(cursor++)) & kStripLengthMask;
        vertex(context, output, input, points, normals, cursor, uv_cursor, color);
        output.byte_offset += kNormalUvVertexBytes;
        for (std::uint32_t index = 0; index != count; ++index) {
            vertex(context, output, input, points, normals, cursor++, uv_cursor, color);
            uv_cursor += 2;
            output.byte_offset += kNormalUvVertexBytes;
        }
        // Original quirk: a zero-length strip still reads the header as its trailing
        // index and the preceding UV pair. No validity guard or winding-bit handling.
        vertex(context, output, input, points, normals, cursor - 1, uv_cursor - 2, color);
        output.byte_offset += kNormalUvVertexBytes;
    }
    unlock(context, buffer);
    draw_triangle_strip(context, buffer);
}
} // namespace sadx::polybuff
