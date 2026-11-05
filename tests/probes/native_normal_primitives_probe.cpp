#include "engine/normal_primitives.hpp"
namespace sadx::polybuff {
// Reference-only selection among the three source normal-only table entries.
void normal_primitive_probe(Context &, BufferState &, const Mesh &, const Vectors &,
                              const Vectors &, std::uint32_t, std::uint32_t);
}
#define normal_uv_strip normal_primitive_probe
#define sadx_native_normal_uv_strip_probe sadx_native_normal_primitives_probe
#include "native_normal_uv_strip_probe.cpp"
#undef normal_uv_strip
namespace sadx::polybuff {
void normal_primitive_probe(Context &context, BufferState &buffer, const Mesh &mesh,
                              const Vectors &points, const Vectors &normals, std::uint32_t color,
                              std::uint32_t cull_mode) {
    if (context.input[26] == 0)
        normal_strip(context, buffer, mesh, points, normals, color, cull_mode);
    else if (context.input[26] == 1)
        normal_triangles(context, buffer, mesh, points, normals, color, cull_mode);
    else
        normal_quads(context, buffer, mesh, points, normals, color, cull_mode);
}
}
