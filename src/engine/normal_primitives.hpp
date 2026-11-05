#pragma once
#include "engine/normal_uv_quad.hpp"

namespace sadx::polybuff {
constexpr std::uint32_t kNormalVertexBytes = 28;
// FUNCTION: SONIC 0x783270 polybuff_normal_strip
void normal_strip(Context &, BufferState &, const Mesh &, const Vectors &, const Vectors &,
                  std::uint32_t color, std::uint32_t cull_mode);
// FUNCTION: SONIC 0x783410 polybuff_normal_tri
void normal_triangles(Context &, BufferState &, const Mesh &, const Vectors &, const Vectors &,
                      std::uint32_t color, std::uint32_t cull_mode);
// FUNCTION: SONIC 0x7834c0 polybuff_normal_quad
void normal_quads(Context &, BufferState &, const Mesh &, const Vectors &, const Vectors &,
                  std::uint32_t color, std::uint32_t cull_mode);
} // namespace sadx::polybuff
