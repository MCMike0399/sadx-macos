#include "engine/normal_primitives.hpp"
#include <array>

namespace sadx::polybuff {
namespace {
// NOTSA: shared source component-copy sequence for normal-only primitive entries.
void normal_vertex(Context &context, VertexCursor output, std::int16_t index, const Vectors &points,
                   const Vectors &normals, std::uint32_t color) {
    const auto first = std::int64_t(index) * 3;
    for (unsigned axis = 0; axis < 3; ++axis)
        store_vertex_word(context, output, axis, points.components.at(first + axis));
    for (unsigned axis = 0; axis < 3; ++axis)
        store_vertex_word(context, output, axis + 3, normals.components.at(first + axis));
    store_vertex_word(context, output, 6, color);
}
} // namespace
void normal_strip(Context &context, BufferState &buffer, const Mesh &mesh, const Vectors &points,
                  const Vectors &normals, std::uint32_t color, std::uint32_t cull_mode) {
    const auto input = mesh;
    std::uint32_t vertices = 0;
    std::int64_t cursor = 0;
    for (unsigned strip = 0; strip < input.strip_count; ++strip) {
        const auto count = std::uint16_t(input.indices.at(cursor)) & kStripLengthMask;
        vertices += count + 2;
        cursor += count + 1;
    }
    set_current(context, buffer);
    auto output = lock_triangle_strip(context, buffer, vertices, cull_mode);
    cursor = 0;
    const auto strip_count = mesh.strip_count;
    for (unsigned strip = 0; strip < strip_count; ++strip) {
        const auto count = std::uint16_t(input.indices.at(cursor++)) & kStripLengthMask;
        normal_vertex(context, output, input.indices.at(cursor), points, normals, color);
        output.byte_offset += kNormalVertexBytes;
        for (unsigned corner = 0; corner < count; ++corner) {
            normal_vertex(context, output, input.indices.at(cursor++), points, normals, color);
            output.byte_offset += kNormalVertexBytes;
        }
        // Zero-length strips still consume the preceding header as a trailing index.
        normal_vertex(context, output, input.indices.at(cursor - 1), points, normals, color);
        output.byte_offset += kNormalVertexBytes;
    }
    unlock(context, buffer);
    draw_triangle_strip(context, buffer);
}
void normal_quads(Context &context, BufferState &buffer, const Mesh &mesh, const Vectors &points,
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
            for (unsigned triangle = 0; triangle < 2; ++triangle) {
                for (unsigned corner = 0; corner < 3; ++corner) {
                    normal_vertex(context, output, corners[triangle + corner], points, normals,
                                  color);
                    output.byte_offset += kNormalVertexBytes;
                }
            }
        }
    }
    unlock(context, buffer);
    draw_triangle_list(context, buffer);
}
void normal_triangles(Context &context, BufferState &buffer, const Mesh &mesh,
                      const Vectors &points, const Vectors &normals, std::uint32_t color,
                      std::uint32_t cull_mode) {
    const auto input = mesh;
    const auto vertices = std::uint32_t(input.strip_count) * 3;
    set_current(context, buffer);
    auto output = lock_triangle_list(context, buffer, vertices, cull_mode);
    if (output.memory || output.byte_offset != 0) {
        for (std::uint32_t vertex = 0; vertex < vertices; ++vertex) {
            normal_vertex(context, output, input.indices.at(vertex), points, normals, color);
            output.byte_offset += kNormalVertexBytes;
        }
    }
    unlock(context, buffer);
    draw_triangle_list(context, buffer);
}
} // namespace sadx::polybuff
