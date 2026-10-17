#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace sadx::graphics {
// NOTSA: owned device commands shared by reference decoding and native Ninja submission.
using Matrix = std::array<float, 16>;
constexpr Matrix kIdentity = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
enum class PositionSpace { Object, Screen };
struct Viewport {
    float x = 0, y = 0, width = 0, height = 0, near_depth = 0, far_depth = 1;
};
struct Vertex {
    std::array<float, 4> position = {0, 0, 0, 1};
    std::array<float, 4> diffuse = {1, 1, 1, 1};
    std::array<float, 4> specular = {0, 0, 0, 0};
    std::array<float, 3> normal = {0, 0, 0};
    std::array<std::array<float, 2>, 2> uv{};
};
enum class TextureFormat { Bgra8, Rgba32Float };
struct Texture {
    std::uint32_t width = 0, height = 0;
    TextureFormat format = TextureFormat::Bgra8;
    std::vector<std::uint8_t> bgra;
    std::vector<float> rgba;
    std::vector<Texture> mipmaps;
};
enum class LightType : std::uint32_t { Point = 1, Spot = 2, Directional = 3 };
struct Material {
    std::array<float, 4> diffuse = {1, 1, 1, 1};
    std::array<float, 4> ambient{}, specular{}, emissive{};
    float power = 0;
};
struct Light {
    LightType type = LightType::Directional;
    std::array<float, 4> diffuse{}, specular{}, ambient{};
    std::array<float, 3> position{}, direction{};
    float range = 0, falloff = 0;
    std::array<float, 3> attenuation{};
    float theta = 0, phi = 0;
};
struct Draw {
    PositionSpace space = PositionSpace::Object;
    std::uint32_t primitive_type = 4, primitive_count = 0, texture_coordinates = 0;
    bool has_normals = false, has_diffuse = true, has_specular = true;
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    Matrix world = kIdentity, view = kIdentity, projection = kIdentity;
    Matrix texture_transform = kIdentity;
    Material material;
    std::vector<Light> lights;
    Viewport viewport;
    // Direct3D device state IDs, not process addresses. Absent entries use API defaults.
    std::map<std::uint32_t, std::uint32_t> render_states, stage0_states;
    std::shared_ptr<const Texture> texture;
    std::string unsupported;
};
struct Clear {
    std::uint32_t flags = 0, color = 0, z_bits = 0, stencil = 0;
};
struct Frame {
    std::vector<std::variant<Clear, Draw>> commands;
};
struct PreparedVertex {
    std::array<float, 4> clip;
    std::array<float, 4> diffuse, specular;
    std::array<std::array<float, 2>, 2> uv;
    float eye_depth = 0, texture_q = 1;
};
struct PreparedDraw {
    std::vector<PreparedVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::string unsupported;
};
// NOTSA: FVF decoding consumes owned API bytes, never a guest address or executable.
std::string decode_vertices(Draw &draw, std::uint32_t fvf, const std::vector<std::uint8_t> &bytes,
                            std::uint32_t stride);
// NOTSA: fixed-function row-vector transforms and D3D pixel-center conversion for Metal.
PreparedDraw prepare_draw(const Draw &draw);
} // namespace sadx::graphics
