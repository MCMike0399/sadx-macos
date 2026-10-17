#include "engine/graphics_frame_internal.hpp"
#include <algorithm>
#include <cmath>

namespace sadx::graphics {
namespace {
using Vector = std::array<float, 3>;
// NOTSA: small vector operations for the platform lighting contract.
float dot(const Vector &left, const Vector &right) {
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}
// NOTSA: zero-length light/view vectors contribute no directional term.
Vector normalize(Vector value) {
    const auto length = std::sqrt(dot(value, value));
    if (length > 0) {
        for (auto &coordinate : value)
            coordinate /= length;
    }
    return value;
}
// NOTSA: transform a light point/direction from world to camera space.
Vector camera_vector(const Vector &value, const Matrix &view, bool point) {
    Vector result{};
    for (unsigned column = 0; column < 3; ++column) {
        for (unsigned row = 0; row < 3; ++row)
            result[column] += value[row] * view[row * 4 + column];
        if (point)
            result[column] += view[12 + column];
    }
    return result;
}
// NOTSA: API state defaults are defined by Direct3D, not missing fixture facts.
std::uint32_t state(const Draw &draw, unsigned key, unsigned fallback) {
    const auto found = draw.render_states.find(key);
    return found == draw.render_states.end() ? fallback : found->second;
}
// NOTSA: COLOR1/COLOR2 fall back to material when that vertex element is absent.
std::array<float, 4> material_color(const Draw &draw, const Vertex &vertex, unsigned source,
                                    const std::array<float, 4> &material) {
    if (state(draw, 141, 1)) {
        if (source == 1 && draw.has_diffuse)
            return vertex.diffuse;
        if (source == 2 && draw.has_specular)
            return vertex.specular;
    }
    return material;
}
// NOTSA: row-vector normal transform is the inverse transpose of world * view.
bool camera_normal(const Draw &draw, const Vector &normal, Vector &result) {
    float matrix[9] = {};
    for (unsigned row = 0; row < 3; ++row) {
        for (unsigned column = 0; column < 3; ++column) {
            for (unsigned axis = 0; axis < 3; ++axis)
                matrix[row * 3 + column] +=
                    draw.world[row * 4 + axis] * draw.view[axis * 4 + column];
        }
    }
    const auto a = matrix[0], b = matrix[1], c = matrix[2];
    const auto d = matrix[3], e = matrix[4], f = matrix[5];
    const auto g = matrix[6], h = matrix[7], i = matrix[8];
    const float cofactor[9] = {e * i - f * h, f * g - d * i, d * h - e * g,
                               c * h - b * i, a * i - c * g, b * g - a * h,
                               b * f - c * e, c * d - a * f, a * e - b * d};
    const auto determinant = a * cofactor[0] + b * cofactor[1] + c * cofactor[2];
    if (determinant == 0 || !std::isfinite(determinant))
        return false;
    for (unsigned column = 0; column < 3; ++column) {
        for (unsigned row = 0; row < 3; ++row)
            result[column] += normal[row] * cofactor[row * 3 + column] / determinant;
    }
    if (state(draw, 143, 0))
        result = normalize(result);
    return true;
}
// NOTSA: point attenuation and optional spotlight cone, in camera space.
float attenuation(const Light &light, const Vector &to_light, float distance, const Matrix &view) {
    if (light.type == LightType::Directional)
        return 1;
    if (distance > light.range)
        return 0;
    const auto denominator = light.attenuation[0] + light.attenuation[1] * distance +
                             light.attenuation[2] * distance * distance;
    if (!(denominator > 0))
        return 0;
    float factor = 1 / denominator;
    if (light.type == LightType::Spot) {
        const auto direction = normalize(camera_vector(light.direction, view, false));
        const auto rho = -dot(to_light, direction);
        const auto inner = std::cos(light.theta / 2), outer = std::cos(light.phi / 2);
        if (rho <= outer)
            return 0;
        if (rho < inner)
            factor *= std::pow((rho - outer) / (inner - outer), light.falloff);
    }
    return factor;
}
} // namespace
bool normal_to_camera(const Draw &draw, const Vector &normal, Vector &result) {
    return camera_normal(draw, normal, result);
}
std::string light_vertex(const Draw &draw, const Vertex &vertex, const std::array<float, 4> &eye,
                         PreparedVertex &output) {
    if (!state(draw, 137, 1))
        return {};
    Vector normal{};
    if (draw.has_normals && !camera_normal(draw, vertex.normal, normal))
        return "singular lighting normal transform";
    const auto diffuse = material_color(draw, vertex, state(draw, 145, 1), draw.material.diffuse);
    const auto ambient = material_color(draw, vertex, state(draw, 147, 0), draw.material.ambient);
    const auto specular = material_color(draw, vertex, state(draw, 146, 2), draw.material.specular);
    const auto emissive = material_color(draw, vertex, state(draw, 148, 0), draw.material.emissive);
    const auto packed_ambient = state(draw, 139, 0);
    const Vector global_ambient = {float((packed_ambient >> 16) & 255) / 255,
                                   float((packed_ambient >> 8) & 255) / 255,
                                   float(packed_ambient & 255) / 255};
    output.diffuse = {0, 0, 0, diffuse[3]};
    output.specular = {0, 0, 0, vertex.specular[3]};
    for (unsigned channel = 0; channel < 3; ++channel)
        output.diffuse[channel] = emissive[channel] + ambient[channel] * global_ambient[channel];
    const auto view_direction =
        state(draw, 142, 1) ? normalize(Vector{-eye[0], -eye[1], -eye[2]}) : Vector{0, 0, -1};
    for (const auto &light : draw.lights) {
        Vector direction{};
        float distance = 0;
        if (light.type == LightType::Directional) {
            direction = camera_vector(light.direction, draw.view, false);
            for (auto &coordinate : direction)
                coordinate = -coordinate;
        } else {
            direction = camera_vector(light.position, draw.view, true);
            for (unsigned axis = 0; axis < 3; ++axis)
                direction[axis] -= eye[axis];
            distance = std::sqrt(dot(direction, direction));
        }
        direction = normalize(direction);
        const auto falloff = attenuation(light, direction, distance, draw.view);
        const auto lambert = std::max(0.0F, dot(normal, direction));
        float reflection = 0;
        if (state(draw, 29, 0) && lambert > 0) {
            Vector halfway{};
            for (unsigned axis = 0; axis < 3; ++axis)
                halfway[axis] = direction[axis] + view_direction[axis];
            reflection =
                std::pow(std::max(0.0F, dot(normal, normalize(halfway))), draw.material.power);
        }
        for (unsigned channel = 0; channel < 3; ++channel) {
            output.diffuse[channel] +=
                falloff * (ambient[channel] * light.ambient[channel] +
                           diffuse[channel] * light.diffuse[channel] * lambert);
            output.specular[channel] +=
                falloff * specular[channel] * light.specular[channel] * reflection;
        }
    }
    for (unsigned channel = 0; channel < 3; ++channel) {
        output.diffuse[channel] = std::clamp(output.diffuse[channel], 0.0F, 1.0F);
        output.specular[channel] = std::clamp(output.specular[channel], 0.0F, 1.0F);
    }
    return {};
}
} // namespace sadx::graphics
