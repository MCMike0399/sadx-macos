#include "engine/normal_uv_quad.hpp"
namespace sadx::polybuff {
// Reference-only selection of the tested source primitive, with unchanged receipts.
void normal_uv_list_probe(Context &, BufferState &, const Mesh &, const Vectors &,
                           const Vectors &, std::uint32_t, std::uint32_t);
}
#define normal_uv_strip normal_uv_list_probe
#define sadx_native_normal_uv_strip_probe sadx_native_normal_uv_quad_probe
#include "native_normal_uv_strip_probe.cpp"
#undef normal_uv_strip
namespace sadx::polybuff {
void normal_uv_list_probe(Context &context, BufferState &buffer, const Mesh &mesh,
                           const Vectors &points, const Vectors &normals, std::uint32_t color,
                           std::uint32_t cull_mode) {
    if (context.input[26])
        normal_uv_triangles(context, buffer, mesh, points, normals, color, cull_mode);
    else
        normal_uv_quad(context, buffer, mesh, points, normals, color, cull_mode);
}
}
