#include "engine/graphics_frame.hpp"
#include "engine/graphics_frame_internal.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace sadx::graphics {
namespace {
// NOTSA: decode the API's packed ARGB color into normalized shader channels.
std::array<float, 4> color(std::uint32_t word) {
    return {float((word >> 16) & 255) / 255, float((word >> 8) & 255) / 255,
            float(word & 255) / 255, float(word >> 24) / 255};
}
// NOTSA: the Direct3D fixed-function contract uses row vectors, world then view then projection.
std::array<float, 4> transform(const std::array<float, 4> &value, const Matrix &matrix) {
    std::array<float, 4> result{};
    for (unsigned column = 0; column < 4; ++column) {
        for (unsigned row = 0; row < 4; ++row)
            result[column] += value[row] * matrix[row * 4 + column];
    }
    return result;
}
// NOTSA: stage-zero texture generation and homogeneous texture transforms.
std::string texture_coordinates(const Draw &draw, const Vertex &vertex,
                                const std::array<float, 4> &eye, PreparedVertex &output) {
    const auto found = draw.stage0_states.find(11);
    const auto selector = found == draw.stage0_states.end() ? 0U : found->second;
    const auto channel = selector & 65535U;
    const auto generation = selector >> 16;
    std::array<float, 4> input{0, 0, 0, 1};
    if (generation == 1) {
        std::array<float, 3> normal{};
        if (!normal_to_camera(draw, vertex.normal, normal))
            return "singular texture normal transform";
        std::copy(normal.begin(), normal.end(), input.begin());
    } else if (generation == 2) {
        input = eye;
    } else if (generation != 0) {
        return "unsupported texture coordinate generation";
    } else {
        if (channel >= 2)
            return "texture coordinate channel";
        input[0] = vertex.uv[channel][0];
        input[1] = vertex.uv[channel][1];
    }
    const auto flags = draw.stage0_states.find(24);
    const auto mode = flags == draw.stage0_states.end() ? 0U : flags->second;
    if (mode) {
        const auto count = mode & 255U;
        if (count < 1 || count > 4 || (mode & ~511U))
            return "texture transform flags";
        input = transform(input, draw.texture_transform);
        if (mode & 256U) {
            if (count < 2)
                return "projected texture coordinate count";
            output.texture_q = input[count - 1];
            if (output.texture_q == 0 || !std::isfinite(output.texture_q))
                return "invalid texture projection divisor";
            if (count == 2)
                input[1] = 0;
        } else if (count == 1) {
            input[1] = 0;
        }
    }
    output.uv[0] = {input[0], input[1]};
    return {};
}

// NOTSA: transform and light vertices before topology conversion.
std::string prepare_vertices(const Draw &draw, PreparedDraw &result, bool valid_normals) {
    const auto &viewport = draw.viewport;
    for (const auto &vertex : draw.vertices) {
        PreparedVertex output{vertex.position, vertex.diffuse, vertex.specular, vertex.uv, 0, 1};
        if (draw.space == PositionSpace::Object) {
            const auto world = transform(vertex.position, draw.world);
            const auto eye = transform(world, draw.view);
            output.eye_depth = eye[2];
            if (const auto problem =
                    valid_normals ? light_vertex(draw, vertex, eye, output) : std::string{};
                !problem.empty()) {
                return problem;
            }
            output.clip = transform(eye, draw.projection);
            if (valid_normals && (draw.texture_coordinates || draw.texture)) {
                if (auto problem = texture_coordinates(draw, vertex, eye, output);
                    !problem.empty()) {
                    return problem;
                }
            }
            // D3D raster centers are integers; Metal's are half-integers.
            output.clip[0] += output.clip[3] / viewport.width;
            output.clip[1] -= output.clip[3] / viewport.height;
        } else {
            if (!(vertex.position[3] > 0)) {
                return "invalid XYZRHW reciprocal W";
            }
            const float clip_w = 1 / vertex.position[3];
            output.eye_depth = clip_w;
            if (draw.texture_coordinates || draw.texture) {
                const auto mode = draw.stage0_states.find(11);
                if (mode != draw.stage0_states.end() && (mode->second >> 16)) {
                    return "screen-space texture generation";
                }
                if (auto problem = texture_coordinates(draw, vertex, vertex.position, output);
                    !problem.empty()) {
                    return problem;
                }
            }
            output.clip = {
                ((vertex.position[0] + 0.5F - viewport.x) / viewport.width * 2 - 1) * clip_w,
                (1 - (vertex.position[1] + 0.5F - viewport.y) / viewport.height * 2) * clip_w,
                (vertex.position[2] - viewport.near_depth) /
                    (viewport.far_depth - viewport.near_depth) * clip_w,
                clip_w};
        }
        for (const auto value : output.clip) {
            if (!std::isfinite(value)) {
                return "nonfinite clip position";
            }
        }
        for (const auto value : output.diffuse) {
            if (!std::isfinite(value))
                return "nonfinite diffuse color";
        }
        for (const auto value : output.uv[0]) {
            if (!std::isfinite(value))
                return "nonfinite texture coordinate";
        }
        result.vertices.push_back(output);
    }
    return {};
}
// NOTSA: normalize lists, alternating strips and fans to one indexed triangle stream.
std::string prepare_indices(const Draw &draw, PreparedDraw &result) {
    const auto count = draw.indices.empty() ? draw.vertices.size() : draw.indices.size();
    const std::uint64_t required = draw.primitive_type == 4
                                       ? std::uint64_t(draw.primitive_count) * 3
                                       : std::uint64_t(draw.primitive_count) + 2;
    if (draw.primitive_type < 4 || draw.primitive_type > 6 || required > count) {
        return "primitive count or type";
    }
    const auto index = [&](std::uint32_t at) {
        return draw.indices.empty() ? at : draw.indices[at];
    };
    for (std::uint32_t primitive = 0; primitive < draw.primitive_count; ++primitive) {
        std::array<std::uint32_t, 3> triangle{};
        if (draw.primitive_type == 4)
            triangle = {index(primitive * 3), index(primitive * 3 + 1), index(primitive * 3 + 2)};
        else if (draw.primitive_type == 5)
            triangle = {index(primitive + (primitive % 2)), index(primitive + 1 - (primitive % 2)),
                        index(primitive + 2)};
        else
            triangle = {index(0), index(primitive + 1), index(primitive + 2)};
        for (const auto vertex : triangle) {
            if (vertex >= draw.vertices.size()) {
                return "index outside vertex range";
            }
            result.indices.push_back(vertex);
        }
    }
    return {};
}
// NOTSA: exact homogeneous area identifies raster no-ops without choosing unknown lighting.
bool zero_raster_area(const PreparedDraw &draw) {
    for (std::size_t index = 0; index < draw.indices.size(); index += 3) {
        const auto &a = draw.vertices[draw.indices[index]].clip;
        const auto &b = draw.vertices[draw.indices[index + 1]].clip;
        const auto &c = draw.vertices[draw.indices[index + 2]].clip;
        const auto area = a[0] * (b[1] * c[3] - b[3] * c[1]) - a[1] * (b[0] * c[3] - b[3] * c[0]) +
                          a[3] * (b[0] * c[1] - b[1] * c[0]);
        if (area != 0)
            return false;
    }
    return true;
}

} // namespace
std::string decode_vertices(Draw &draw, std::uint32_t fvf, const std::vector<std::uint8_t> &bytes,
                            std::uint32_t stride) {
    constexpr std::uint32_t kPositionMask = 14, kXyz = 2, kXyzRhw = 4;
    constexpr std::uint32_t kNormal = 16, kDiffuse = 64, kSpecular = 128, kTextureMask = 3840;
    const auto position = fvf & kPositionMask;
    const auto textures = (fvf & kTextureMask) >> 8;
    if ((position != kXyz && position != kXyzRhw) || textures > 2 ||
        (fvf & ~(kPositionMask | kNormal | kDiffuse | kSpecular | kTextureMask)))
        return "FVF " + std::to_string(fvf);
    if (position == kXyzRhw && (fvf & kNormal))
        return "XYZRHW with normals";
    const auto position_bytes = position == kXyz ? 12U : 16U;
    const auto required = position_bytes + ((fvf & kNormal) ? 12 : 0) + ((fvf & kDiffuse) ? 4 : 0) +
                          ((fvf & kSpecular) ? 4 : 0) + textures * 8;
    if (stride < required || bytes.size() % stride != 0)
        return "truncated FVF vertex layout";
    draw.space = position == kXyz ? PositionSpace::Object : PositionSpace::Screen;
    draw.has_normals = (fvf & kNormal) != 0;
    draw.has_diffuse = (fvf & kDiffuse) != 0;
    draw.has_specular = (fvf & kSpecular) != 0;
    draw.texture_coordinates = textures;
    draw.vertices.clear();
    for (std::size_t offset = 0; offset < bytes.size(); offset += stride) {
        Vertex vertex;
        unsigned field = 0;
        const auto read = [&](void *target, unsigned size) {
            std::memcpy(target, bytes.data() + offset + field, size);
            field += size;
        };
        read(vertex.position.data(), position_bytes);
        if (draw.has_normals)
            read(vertex.normal.data(), 12);
        if (fvf & kDiffuse) {
            std::uint32_t packed = 0;
            read(&packed, 4);
            vertex.diffuse = color(packed);
        }
        if (fvf & kSpecular) {
            std::uint32_t packed = 0;
            read(&packed, 4);
            vertex.specular = color(packed);
        }
        for (unsigned channel = 0; channel < textures; ++channel)
            read(vertex.uv[channel].data(), 8);
        draw.vertices.push_back(vertex);
    }
    return {};
}
PreparedDraw prepare_draw(const Draw &draw) {
    PreparedDraw result;
    const auto &viewport = draw.viewport;
    if (!(viewport.width > 0 && viewport.height > 0 && viewport.far_depth > viewport.near_depth)) {
        result.unsupported = "invalid viewport";
        return result;
    }
    const auto lighting = draw.render_states.find(137);
    const auto coordinates = draw.stage0_states.find(11);
    const bool normals_used =
        draw.has_normals && draw.space == PositionSpace::Object &&
        ((lighting == draw.render_states.end() || lighting->second) ||
         (coordinates != draw.stage0_states.end() && (coordinates->second >> 16) == 1));
    std::array<float, 3> ignored_normal{};
    const bool valid_normals = !normals_used || normal_to_camera(draw, {0, 0, 0}, ignored_normal);
    result.unsupported = prepare_vertices(draw, result, valid_normals);
    if (!result.unsupported.empty())
        return result;
    result.unsupported = prepare_indices(draw, result);
    if (!result.unsupported.empty())
        return result;
    if (!valid_normals) {
        if (!zero_raster_area(result))
            result.unsupported = "singular visible normal transform";
        else
            result.indices.clear();
    }
    return result;
}
} // namespace sadx::graphics
