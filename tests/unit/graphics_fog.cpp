#include "engine/graphics_frame.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace sadx::graphics;
namespace {
void require(bool value, const char *why) {
    if (!value)
        throw std::runtime_error(why);
}
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
Draw triangle() {
    Draw draw;
    draw.viewport = {0, 0, 64, 64, 0, 1};
    draw.primitive_count = 1;
    draw.render_states = {{28, 1}, {140, 3}, {36, bits(0)}, {37, bits(10)}, {137, 0}};
    for (auto position :
         std::array<std::array<float, 4>, 3>{{{3, 4, 0, 1}, {0, 0, -5, 1}, {0, 0, 12, 1}}}) {
        Vertex vertex;
        vertex.position = position;
        vertex.specular = {0.1F, 0.2F, 0.3F, 0.25F};
        vertex.diffuse[3] = 0.75F;
        draw.vertices.push_back(vertex);
    }
    return draw;
}
PreparedDraw prepared(const Draw &draw) {
    auto result = prepare_draw(draw);
    require(result.unsupported.empty(), result.unsupported.c_str());
    for (const auto &v : result.vertices) {
        require(v.diffuse[3] == 0.75F, "fog preserves opacity");
        require(v.specular == draw.vertices[0].specular, "fog preserves specular color input");
    }
    return result;
}
} // namespace
int main() {
    try {
        auto draw = triangle();
        auto result = prepared(draw);
        require(result.vertices[0].fog_factor == 1 && result.vertices[1].fog_factor == 0.5F &&
                    result.vertices[2].fog_factor == 0,
                "linear absolute camera Z and clamping");
        draw.render_states[48] = 1;
        require(prepared(draw).vertices[0].fog_factor == 0.5F, "range fog uses camera XYZ length");
        draw.world[12] = -3;
        draw.view[13] = -4;
        require(prepared(draw).vertices[0].fog_factor == 1, "range fog after world and view");
        draw = triangle();
        draw.render_states[38] = bits(0.25F);
        draw.render_states[140] = 1;
        require(std::abs(prepared(draw).vertices[1].fog_factor - std::exp(-1.25F)) < 0.000001F,
                "exponential vertex fog");
        draw.render_states[140] = 2;
        require(std::abs(prepared(draw).vertices[1].fog_factor - std::exp(-1.5625F)) < 0.000001F,
                "squared exponential vertex fog");
        draw.render_states[140] = 3;
        draw.render_states[36] = bits(10);
        draw.render_states[37] = bits(0);
        require(prepared(draw).vertices[1].fog_factor == 0.5F, "reversed finite interval");
        draw.render_states[37] = bits(10);
        require(prepare_draw(draw).unsupported == "zero vertex fog interval", "unknown interval");
        draw.render_states[37] = bits(std::numeric_limits<float>::quiet_NaN());
        require(prepare_draw(draw).unsupported == "nonfinite vertex fog interval", "NaN interval");
        draw.render_states[140] = 1;
        draw.render_states[38] = bits(-1);
        require(!prepare_draw(draw).unsupported.empty(), "invalid density rejected");
        draw.render_states[140] = 4;
        require(prepare_draw(draw).unsupported == "vertex fog mode", "unknown active formula");
        draw.render_states[35] = 3;
        require(prepared(draw).vertices[0].fog_factor == 1, "table fog overrides vertex states");
        draw.render_states[35] = 0;
        draw.render_states[28] = 0;
        require(prepared(draw).vertices[0].fog_factor == 1, "disabled fog ignores parameters");
        draw.render_states[28] = 1;
        draw.render_states[140] = 0;
        require(prepared(draw).vertices[0].fog_factor == 0.25F, "supplied specular alpha");
        draw.has_specular = false;
        require(prepared(draw).vertices[0].fog_factor == 1, "absent specular means no vertex fog");
        draw.has_specular = true;
        draw.space = PositionSpace::Screen;
        draw.render_states[140] = 3;
        require(prepared(draw).vertices[0].fog_factor == 0.25F,
                "pretransformed vertices supply fog; formula parameters ignored");
        std::cout
            << "graphics fog: formulas, range, supplied factors, precedence and failures pass\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
