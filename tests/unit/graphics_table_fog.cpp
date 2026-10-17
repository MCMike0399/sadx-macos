#include "engine/graphics_frame.hpp"
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
} // namespace
int main() {
    try {
        Draw draw;
        draw.render_states = {{28, 1}, {35, 3}};
        for (auto space : {PositionSpace::Object, PositionSpace::Screen}) {
            draw.space = space;
            draw.projection = kIdentity;
            auto fog = table_fog(draw);
            require(fog.unsupported.empty() && fog.depth == FogDepthSource::DeviceZ &&
                        fog.mode == 3 && fog.start == 0 && fog.end == 1,
                    "affine projection selects device Z");
            draw.projection[11] = 1;
            draw.projection[15] = 0;
            require(table_fog(draw).depth == FogDepthSource::EyeW,
                    "current projection selects W for both FVFs");
            for (auto index : {3, 7, 11, 15}) {
                const auto saved = draw.projection[index];
                draw.projection[index] = 0.5F;
                require(table_fog(draw).unsupported == "table fog projection",
                        "noncanonical projection explicit");
                draw.projection[index] = saved;
            }
        }
        draw.render_states[36] = bits(5);
        draw.render_states[37] = bits(-5);
        auto fog = table_fog(draw);
        require(fog.unsupported.empty() && fog.start == 5 && fog.end == -5,
                "reversed finite interval");
        draw.render_states[37] = bits(5);
        require(table_fog(draw).unsupported == "zero fog interval",
                "zero interval remains explicit");
        for (float bad :
             {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
            draw.render_states[37] = bits(bad);
            require(table_fog(draw).unsupported == "nonfinite table fog interval",
                    "nonfinite interval rejected");
        }
        for (unsigned mode : {1U, 2U}) {
            draw.render_states[35] = mode;
            draw.render_states[38] = bits(0.25F);
            fog = table_fog(draw);
            require(fog.unsupported.empty() && fog.mode == mode && fog.density == 0.25F,
                    "exponential modes ignore invalid inactive interval");
            for (float bad : {-0.1F, 1.1F, std::numeric_limits<float>::infinity(),
                              std::numeric_limits<float>::quiet_NaN()}) {
                draw.render_states[38] = bits(bad);
                require(table_fog(draw).unsupported == "table fog density outside [0,1]",
                        "bad active density explicit");
            }
        }
        draw.render_states[35] = 3;
        draw.render_states[36] = bits(0);
        draw.render_states[37] = bits(1);
        require(table_fog(draw).unsupported.empty(),
                "linear mode ignores invalid inactive density");
        draw.render_states[35] = 4;
        require(table_fog(draw).unsupported == "fog mode", "unknown formula explicit");
        draw.render_states[28] = 0;
        draw.projection[15] = std::numeric_limits<float>::quiet_NaN();
        fog = table_fog(draw);
        require(fog.unsupported.empty() && fog.mode == 0 && fog.depth == FogDepthSource::Disabled,
                "disabled fog ignores inactive parameters/projection");
        draw.render_states[28] = 1;
        draw.render_states[35] = 0;
        require(table_fog(draw).unsupported.empty() &&
                    table_fog(draw).depth == FogDepthSource::Disabled,
                "vertex fog does not require table projection");
        std::cout << "Table fog state: Z/W selection, both vertex spaces, formulas, active "
                     "parameters and explicit unknowns pass\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
