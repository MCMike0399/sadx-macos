#include "engine/graphics_frame_internal.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace sadx::graphics {
namespace {
// NOTSA: decode the fixed-function API's integer and bit-cast float render states.
std::uint32_t state(const Draw &draw, std::uint32_t key, std::uint32_t fallback = 0) {
    const auto found = draw.render_states.find(key);
    return found == draw.render_states.end() ? fallback : found->second;
}
// NOTSA: SetRenderState carries floating-point fog parameters as DWORD bits.
float parameter(const Draw &draw, std::uint32_t key, float fallback) {
    const auto found = draw.render_states.find(key);
    if (found == draw.render_states.end())
        return fallback;
    float value;
    std::memcpy(&value, &found->second, sizeof(value));
    return value;
}
} // namespace
TableFog table_fog(const Draw &draw) {
    TableFog result;
    if (!state(draw, 28) || !state(draw, 35))
        return result;
    result.mode = state(draw, 35);
    if (result.mode > 3) {
        result.unsupported = "fog mode";
        return result;
    }
    const auto &projection = draw.projection;
    if (projection[3] == 0 && projection[7] == 0 && projection[11] == 0 && projection[15] == 1)
        result.depth = FogDepthSource::DeviceZ;
    else if (projection[3] == 0 && projection[7] == 0 && projection[11] == 1 && projection[15] == 0)
        result.depth = FogDepthSource::EyeW;
    else {
        result.unsupported = "table fog projection";
        return result;
    }
    if (result.mode == 3) {
        result.start = parameter(draw, 36, 0);
        result.end = parameter(draw, 37, 1);
        if (!std::isfinite(result.start) || !std::isfinite(result.end))
            result.unsupported = "nonfinite table fog interval";
        else if (result.start == result.end)
            result.unsupported = "zero fog interval";
    } else {
        result.density = parameter(draw, 38, 1);
        if (!std::isfinite(result.density) || result.density < 0 || result.density > 1)
            result.unsupported = "table fog density outside [0,1]";
    }
    return result;
}
std::string vertex_fog(const Draw &draw, const Vertex &vertex, const std::array<float, 4> &eye,
                       float &factor) {
    factor = 1;
    if (!state(draw, 28) || state(draw, 35))
        return {};
    const auto mode = state(draw, 140);
    if (draw.space == PositionSpace::Screen || mode == 0) {
        factor = draw.has_specular ? vertex.specular[3] : 1;
        if (!std::isfinite(factor))
            return "nonfinite supplied vertex fog";
        factor = std::clamp(factor, 0.0F, 1.0F);
        return {};
    }
    if (mode > 3)
        return "vertex fog mode";
    // Microsoft Fog Formulas: absolute camera Z, or Euclidean camera distance.
    const float distance = state(draw, 48)
                               ? std::sqrt(eye[0] * eye[0] + eye[1] * eye[1] + eye[2] * eye[2])
                               : std::abs(eye[2]);
    if (!std::isfinite(distance))
        return "nonfinite vertex fog distance";
    if (mode == 3) {
        const float start = parameter(draw, 36, 0), end = parameter(draw, 37, 1);
        if (!std::isfinite(start) || !std::isfinite(end))
            return "nonfinite vertex fog interval";
        if (start == end)
            return "zero vertex fog interval";
        factor = (end - distance) / (end - start);
    } else {
        const float density = parameter(draw, 38, 1);
        if (!std::isfinite(density) || density < 0 || density > 1)
            return "vertex fog density outside [0,1]";
        const float scaled = density * distance;
        factor = std::exp(-(mode == 2 ? scaled * scaled : scaled));
    }
    factor = std::clamp(factor, 0.0F, 1.0F);
    return {};
}
} // namespace sadx::graphics
