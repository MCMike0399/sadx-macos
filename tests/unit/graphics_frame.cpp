#include "engine/graphics_frame.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace sadx::graphics;
namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
bool near(float a, float b) { return std::abs(a - b) < 0.000001F; }
} // namespace
int main() {
    try {
        Draw draw;
        draw.viewport = {0, 0, 100, 100, 0, 1};
        draw.primitive_count = 1;
        draw.vertices.resize(3);
        // Asymmetric world, view and projection make transposition and reversed
        // composition observable independently of the renderer's implementation.
        draw.world = kIdentity;
        draw.world[12] = 2;
        draw.view = kIdentity;
        draw.view[0] = 2;
        draw.view[13] = 3;
        draw.projection = kIdentity;
        draw.projection[10] = 0.5F;
        draw.projection[11] = 1;
        draw.projection[15] = 0;
        for (auto &vertex : draw.vertices)
            vertex.position = {1, 2, 4, 1};
        auto prepared = prepare_draw(draw);
        require(prepared.unsupported.empty(), "object prepare");
        const auto clip = prepared.vertices[0].clip;
        require(near(clip[0], 6.04F) && near(clip[1], 4.96F) && clip[2] == 2 && clip[3] == 4,
                "world then view then projection, retained clip W and half-pixel offset");
        require(prepared.vertices[0].eye_depth == 4, "eye depth retained for table fog");
        require(prepared.indices == std::vector<std::uint32_t>{0, 1, 2}, "triangle list");
        draw.space = PositionSpace::Screen;
        for (auto &vertex : draw.vertices)
            vertex.position = {49.5F, 49.5F, 0.25F, 0.5F};
        prepared = prepare_draw(draw);
        require(prepared.vertices[0].clip == std::array<float, 4>{0, 0, 0.5F, 2},
                "XYZRHW reciprocal W");
        draw.primitive_type = 5;
        draw.primitive_count = 2;
        draw.vertices.resize(4);
        prepared = prepare_draw(draw);
        require(prepared.indices == std::vector<std::uint32_t>{0, 1, 2, 2, 1, 3},
                "strip alternating winding");
        draw.primitive_type = 6;
        draw.indices = {3, 2, 1, 0};
        prepared = prepare_draw(draw);
        require(prepared.indices == std::vector<std::uint32_t>{3, 2, 1, 3, 1, 0},
                "indexed fan rebasing");
        draw.indices = {3, 2, 9, 0};
        require(!prepare_draw(draw).unsupported.empty(), "index bounds");
        draw.indices.clear();
        draw.vertices[0].position[3] = 0;
        require(!prepare_draw(draw).unsupported.empty(), "zero reciprocal W");
        draw.space = PositionSpace::Object;
        draw.vertices[0].position[0] = std::numeric_limits<float>::infinity();
        require(!prepare_draw(draw).unsupported.empty(), "nonfinite coordinates");
        struct Packed {
            float xyz[3], normal[3];
            std::uint32_t diffuse, specular;
            float uv[4];
        };
        const Packed packed{
            {1, 2, 3}, {0, 1, 0}, 0x80402010U, 0x11223344U, {0.25F, 0.5F, 0.75F, 1}};
        std::vector<std::uint8_t> bytes(sizeof packed);
        std::memcpy(bytes.data(), &packed, sizeof packed);
        Draw decoded;
        require(decode_vertices(decoded, 2 + 16 + 64 + 128 + 512, bytes, sizeof packed).empty(),
                "XYZ normal diffuse specular TEX2 layout");
        require(decoded.has_normals && decoded.texture_coordinates == 2,
                "decoded vertex capabilities");
        require(decoded.vertices[0].position == std::array<float, 4>{1, 2, 3, 1}, "XYZ width");
        require(decoded.vertices[0].uv[1] == std::array<float, 2>{0.75F, 1}, "second UV offset");
        require(near(decoded.vertices[0].diffuse[0], 64.0F / 255) &&
                    near(decoded.vertices[0].diffuse[3], 128.0F / 255),
                "packed ARGB");
        require(!decode_vertices(decoded, 2 + 64 + 256, bytes, 12).empty(), "short stride");
        require(!decode_vertices(decoded, 2 + 64 + 256, bytes, 0).empty(), "zero stride");
        require(!decode_vertices(decoded, 2 + 768, bytes, sizeof packed).empty(),
                "TEX3 explicit unsupported");
        require(!decode_vertices(decoded, 4 + 16, bytes, sizeof packed).empty(),
                "invalid normal/RHW combination");
        Draw lit;
        lit.viewport = {0, 0, 64, 64, 0, 1};
        lit.primitive_count = 1;
        lit.vertices.resize(3);
        lit.has_normals = true;
        lit.render_states = {{137, 1}, {145, 0}};
        lit.material.diffuse = {1, 1, 1, 0.75F};
        for (auto &vertex : lit.vertices) {
            vertex.position = {0, 0, 0, 1};
            vertex.normal = {0, 0, 1};
        }
        Light point;
        point.type = LightType::Point;
        point.position = {0, 0, 2};
        point.range = 3;
        point.attenuation = {0, 1, 0};
        point.diffuse = {0, 1, 0, 0};
        lit.lights.push_back(point);
        auto lighting = prepare_draw(lit);
        require(lighting.vertices[0].diffuse == std::array<float, 4>{0, 0.5F, 0, 0.75F},
                "point distance attenuation and material alpha");
        lit.lights[0].range = 1;
        lighting = prepare_draw(lit);
        require(lighting.vertices[0].diffuse[1] == 0, "point range excludes vertex");
        lit.lights.clear();
        lit.material.emissive = {0.25F, 0, 0, 0};
        lit.material.ambient = {0, 0.5F, 0, 0};
        lit.render_states[139] = 0x0000ff00U;
        lighting = prepare_draw(lit);
        require(lighting.vertices[0].diffuse == std::array<float, 4>{0.25F, 0.5F, 0, 0.75F},
                "ambient plus emissive");
        lit.render_states[145] = 1;
        lit.vertices[0].diffuse = {1, 0, 0, 0.25F};
        require(prepare_draw(lit).vertices[0].diffuse[3] == 0.25F, "COLOR1 material source");
        lit.has_diffuse = false;
        require(prepare_draw(lit).vertices[0].diffuse[3] == 0.75F,
                "absent COLOR1 falls back to material");
        lit.world[0] = 0;
        auto zero = prepare_draw(lit);
        require(zero.unsupported.empty() && zero.indices.empty(),
                "zero-area singular mesh has no raster coverage");
        lit.world = kIdentity;
        lit.world[10] = 0;
        lit.vertices[0].position = {-0.5F, -0.5F, 0, 1};
        lit.vertices[1].position = {0.5F, -0.5F, 0, 1};
        lit.vertices[2].position = {-0.5F, 0.5F, 0, 1};
        require(!prepare_draw(lit).unsupported.empty(),
                "visible singular normal transform remains unknown");
        Draw generated;
        generated.viewport = {0, 0, 64, 64, 0, 1};
        generated.primitive_count = 1;
        generated.vertices.resize(3);
        generated.has_normals = true;
        generated.texture_coordinates = 1;
        generated.render_states[137] = 0;
        generated.stage0_states = {{11, 65536}, {24, 2}};
        generated.texture_transform[0] = 0.5F;
        generated.texture_transform[5] = -0.5F;
        generated.texture_transform[12] = 0.5F;
        generated.texture_transform[13] = 0.5F;
        for (auto &vertex : generated.vertices) {
            vertex.position = {0, 0, 1, 1};
            vertex.normal = {1, 0, 0};
        }
        auto uv = prepare_draw(generated);
        require(uv.unsupported.empty() && uv.vertices[0].uv[0] == std::array<float, 2>{1, 0.5F},
                "camera normal texture generation and transform");
        generated.stage0_states[11] = 131072;
        generated.stage0_states[24] = 3 + 256;
        generated.texture_transform = kIdentity;
        generated.texture_transform[10] = 2;
        generated.vertices[0].position = {0.5F, 1, 1, 1};
        uv = prepare_draw(generated);
        require(uv.vertices[0].uv[0] == std::array<float, 2>{0.5F, 1} &&
                    uv.vertices[0].texture_q == 2,
                "projected texture divisor retained for fragment interpolation");
        std::cout << "Graphics device: FVF layouts, row-vector transforms, perspective W, pixel "
                     "centers, topology and malformed inputs passed.\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
