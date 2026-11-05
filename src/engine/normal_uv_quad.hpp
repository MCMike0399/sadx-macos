#pragma once
#include "engine/normal_uv_strip.hpp"

namespace sadx::polybuff {
constexpr std::uint32_t kTriangleListPrimitive = 4;
// FUNCTION: SONIC 0x7946c0 PolyBuff_LockTriangleList
VertexCursor lock_triangle_list(Context &, BufferState &, std::uint32_t vertices,
                                std::uint32_t cull_mode);
// FUNCTION: SONIC 0x7947b0 PolyBuff_DrawTriangleList
void draw_triangle_list(Context &, BufferState &);
// Mesh's shared count field denotes quad count for this source table entry.
// FUNCTION: SONIC 0x783920 polybuff_normal_uv_quad
void normal_uv_quad(Context &, BufferState &, const Mesh &, const Vectors &, const Vectors &,
                    std::uint32_t color, std::uint32_t cull_mode);
// FUNCTION: SONIC 0x783830 polybuff_normal_uv_tri
void normal_uv_triangles(Context &, BufferState &, const Mesh &, const Vectors &, const Vectors &,
                         std::uint32_t color, std::uint32_t cull_mode);
} // namespace sadx::polybuff
