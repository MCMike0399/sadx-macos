#include "engine/normal_uv_quad.hpp"
#include <array>
#include <cstring>

namespace sadx::polybuff {
namespace {
// NOTSA: publish only the three words written by the source list allocator.
void list_argument(Context &context, BufferState &buffer, std::uint32_t row, ArgumentField field,
                   std::uint32_t value) {
    auto &destination = buffer.render_arguments.at(row, field);
    observe_argument_store(context, row, field, value);
    destination = value;
}
// NOTSA: the source snapshots all four corner indices before its first vertex store.
void quad_vertex(Context &context, VertexCursor output, std::int16_t index, const Mesh &mesh,
                 const Vectors &points, const Vectors &normals, std::int64_t uv_cursor,
                 std::uint32_t color) {
    const auto first = std::int64_t(index) * 3;
    for (unsigned axis = 0; axis < 3; ++axis)
        store_vertex_word(context, output, axis, points.components.at(first + axis));
    for (unsigned axis = 0; axis < 3; ++axis)
        store_vertex_word(context, output, axis + 3, normals.components.at(first + axis));
    store_vertex_word(context, output, 6, color);
    for (unsigned axis = 0; axis < 2; ++axis) {
        const auto coordinate = mesh.texture_coordinates.at(uv_cursor + axis);
        const auto scaled = (exact(context, float(coordinate)) * kUvScale).rounded();
        std::uint32_t word;
        std::memcpy(&word, &scaled, sizeof(word));
        store_vertex_word(context, output, 7 + axis, word);
    }
}
} // namespace
VertexCursor lock_triangle_list(Context &context, BufferState &buffer, std::uint32_t vertices,
                                std::uint32_t cull_mode) {
    const auto stride = buffer.stride;
    const auto bytes = vertices * stride;
    const bool append = bytes < buffer.remaining_bytes;
    const auto available = append ? buffer.remaining_bytes : buffer.capacity_bytes;
    const auto offset = buffer.capacity_bytes - available;
    observe_buffer_store(context, buffer, BufferField::Remaining, available - bytes);
    buffer.remaining_bytes = available - bytes;
    if (stride == 0)
        throw std::domain_error("polybuffer zero stride division");
    const auto row = buffer.lock_count;
    list_argument(context, buffer, row, ArgumentField::StartVertex, offset / stride);
    list_argument(context, buffer, row, ArgumentField::CullMode, cull_mode);
    list_argument(context, buffer, row, ArgumentField::PrimitiveCount, vertices / 3);
    observe_buffer_store(context, buffer, BufferField::LockCount, buffer.lock_count + 1);
    ++buffer.lock_count;
    VertexCursor output{nullptr, stride};
    lock_vertices(context, buffer.resource, offset, bytes, output,
                  append ? kNoOverwrite : kDiscard);
    return output;
}
void draw_triangle_list(Context &context, BufferState &buffer) {
    const auto count = buffer.lock_count;
    const auto arguments = buffer.render_arguments;
    for (std::uint32_t row = 0; row != count; ++row) {
        set_render_state(context, kCullModeState, arguments.at(row, ArgumentField::CullMode));
        const auto primitive_count = arguments.at(row, ArgumentField::PrimitiveCount);
        const auto start = arguments.at(row, ArgumentField::StartVertex);
        draw_primitive(context, kTriangleListPrimitive, start, primitive_count);
    }
    observe_buffer_store(context, buffer, BufferField::LockCount, 0);
    buffer.lock_count = 0;
}
void normal_uv_quad(Context &context, BufferState &buffer, const Mesh &mesh, const Vectors &points,
                    const Vectors &normals, std::uint32_t color, std::uint32_t cull_mode) {
    const auto input = mesh;
    const auto vertices = std::uint32_t(input.strip_count) * 6;
    set_current(context, buffer);
    auto output = lock_triangle_list(context, buffer, vertices, cull_mode);
    if (output.memory || output.byte_offset != 0) {
        for (unsigned quad = 0; quad < input.strip_count; ++quad) {
            const auto cursor = std::int64_t(quad) * 4;
            const std::array<std::int16_t, 4> corners{
                input.indices.at(cursor), input.indices.at(cursor + 1),
                input.indices.at(cursor + 2), input.indices.at(cursor + 3)};
            // Original quirk: the second triangle starts at corner one, not zero.
            for (unsigned triangle = 0; triangle < 2; ++triangle) {
                for (unsigned corner = 0; corner < 3; ++corner) {
                    const auto selected = triangle + corner;
                    quad_vertex(context, output, corners[selected], input, points, normals,
                                (cursor + selected) * 2, color);
                    output.byte_offset += kNormalUvVertexBytes;
                }
            }
        }
    }
    unlock(context, buffer);
    draw_triangle_list(context, buffer);
}
void normal_uv_triangles(Context &context, BufferState &buffer, const Mesh &mesh,
                         const Vectors &points, const Vectors &normals, std::uint32_t color,
                         std::uint32_t cull_mode) {
    const auto input = mesh;
    const auto vertices = std::uint32_t(input.strip_count) * 3;
    set_current(context, buffer);
    auto output = lock_triangle_list(context, buffer, vertices, cull_mode);
    if (output.memory || output.byte_offset != 0) {
        for (std::uint32_t vertex = 0; vertex < vertices; ++vertex) {
            quad_vertex(context, output, input.indices.at(vertex), input, points, normals,
                        std::int64_t(vertex) * 2, color);
            output.byte_offset += kNormalUvVertexBytes;
        }
    }
    unlock(context, buffer);
    draw_triangle_list(context, buffer);
}
} // namespace sadx::polybuff
