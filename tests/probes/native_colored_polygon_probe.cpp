#include "cpu/fidelity_float.hpp"
#include "engine/pvr_device.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace {
using U = std::uint32_t;
float number(U value) {
    float result;
    std::memcpy(&result, &value, 4);
    return result;
}
U bits(float value) {
    U result;
    std::memcpy(&result, &value, 4);
    return result;
}
constexpr U render_ids[]{27, 22, 137, 145, 14, 23, 9};
constexpr U stage_ids[]{4, 16, 17, 18};
void state_words(const sadx::graphics::Draw &state, U *output) {
    for (auto key : render_ids) {
        const auto found = state.render_states.find(key);
        *output++ = found == state.render_states.end() ? 0 : found->second;
    }
    for (auto key : stage_ids) {
        const auto found = state.stage0_states.find(key);
        *output++ = found == state.stage0_states.end() ? 0 : found->second;
    }
}
U color(const sadx::graphics::Vertex &vertex) {
    const auto channel = [&](unsigned i) {
        return static_cast<U>(std::lround(vertex.diffuse[i] * 255.0f));
    };
    return (channel(3) << 24) | (channel(0) << 16) | (channel(1) << 8) | channel(2);
}
} // namespace
extern "C" void sadx_native_colored_polygon(const U *input, U *output) {
    using namespace sadx;
    std::fill_n(output, 128, 0);
    graphics::Draw state;
    graphics::Frame frame;
    for (unsigned i = 0; i < 7; ++i)
        state.render_states[render_ids[i]] = input[32 + i];
    for (unsigned i = 0; i < 4; ++i)
        state.stage0_states[stage_ids[i]] = input[39 + i];
    state.has_diffuse=input[43]!=0;
    state.has_normals=input[44]!=0;
    state.has_specular=input[45]!=0;
    auto token = std::make_shared<graphics::Texture>();
    if (input[17])
        state.texture = token;
    auto cached = static_cast<std::int32_t>(input[16]);
    bool active = input[17] != 0;
    X87Context fp(static_cast<std::uint16_t>(input[0]));
    unsigned depths = 0, unbinds = 0;
    const auto unbind = [&] {
        const auto offset = 64 + unbinds * 13;
        if (unbinds < 2) {
            state_words(state, output + offset);
            output[offset + 11] = static_cast<U>(cached);
            output[offset + 12] = state.texture != nullptr;
        }
        ++unbinds;
        if (cached != -1) {
            state.texture.reset();
            cached = -1;
            active = false;
        }
    };
    pvr::ImmediateDevice device{state, frame, [&](float argument) {
                                    output[56 + depths] = bits(argument);
                                    ++depths;
                                    if (depths == input[18]) {
                                        state.texture = token;
                                        cached = 99;
                                        active = true;
                                    }
                                    return fp.to_float(fp.from_float(number(input[20 + depths - 1])));
                                }};
    std::array<pvr::Point, 4> points;
    std::array<U, 4> colors;
    for (unsigned i = 0; i < 4; ++i) {
        points[i] = {number(input[4 + i * 2]), number(input[5 + i * 2])};
        colors[i] = input[12 + i];
    }
    try {
        pvr::submit_colored_polygon(points, colors, number(input[2]), input[3], device, fp, unbind);
    } catch (const X87Error &) {
        output[0] = 1;
    }
    output[1] = static_cast<U>(frame.commands.size());
    if (!frame.commands.empty()) {
        const auto &draw = std::get<graphics::Draw>(frame.commands.back());
        output[2] = draw.primitive_type;
        output[3] = draw.primitive_count;
        output[4] = draw.texture != nullptr;
        output[31]=4U|(draw.has_diffuse?64U:0U)|(draw.has_specular?128U:0U)|(draw.has_normals?16U:0U)|(draw.texture_coordinates<<8);
        for (unsigned i = 0; i < 4; ++i) {
            const auto &v = draw.vertices[i];
            const U words[]{bits(v.position[0]), bits(v.position[1]), bits(v.position[2]),
                            bits(v.position[3]), color(v)};
            std::copy_n(words, 5, output + 5 + i * 5);
        }
        state_words(draw, output + 43);
    }
    output[25] = static_cast<U>(cached);
    output[26] = state.texture != nullptr;
    output[27] = active;
    output[28] = fp.exception_flags();
    output[29] = depths;
    output[30] = unbinds;
    state_words(state, output + 32);
}
