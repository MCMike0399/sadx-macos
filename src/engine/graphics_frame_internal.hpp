#pragma once
#include "engine/graphics_frame.hpp"
namespace sadx::graphics {
// NOTSA: shared normal transform for lighting and automatic texture coordinates.
bool normal_to_camera(const Draw &, const std::array<float, 3> &, std::array<float, 3> &);
// NOTSA: Direct3D fixed-function vertex lighting in camera space, before rasterization.
std::string light_vertex(const Draw &, const Vertex &, const std::array<float, 4> &eye,
                         PreparedVertex &output);
// NOTSA: fixed-function vertex fog; table fog is evaluated later by the device.
std::string vertex_fog(const Draw &, const Vertex &, const std::array<float, 4> &eye,
                       float &factor);
} // namespace sadx::graphics
