#include "engine/menu_draw_primitives.hpp"
#include <algorithm>
#include <cstring>
namespace {
float number(std::uint32_t value) {
    float result;
    std::memcpy(&result, &value, 4);
    return result;
}
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
struct Calls final : sadx::pvr::Calls {
    const std::uint32_t *input;
    std::uint32_t *output;
    Calls(const std::uint32_t *i, std::uint32_t *o) : input(i), output(o) {}
    sadx::pvr::TextureInfo texture(std::uint32_t index) override {
        output[28] = index;
        return {input[6], input[7], 257, 255};
    }
    void draw_texture(const std::array<sadx::pvr::Vertex, 4> &, std::uint32_t id,
                      std::uint32_t alpha) override {
        output[24] = 1;
        output[25] = id;
        output[26] = alpha;
    }
    void draw_polygon(const std::array<sadx::pvr::Point, 4> &, std::uint32_t, float,
                      std::uint32_t) override {
        throw std::runtime_error("unexpected polygon");
    }
    void queue_box(float, float, float, float, float, std::uint32_t) override {
        throw std::runtime_error("unexpected queue box");
    }
};
} // namespace
extern "C" void sadx_native_menu_sized(const std::uint32_t *input, std::uint32_t *output) {
    std::fill_n(output, 32, 0);
    sadx::pvr::State state;
    sadx::pvr::initialize(state);
    for (unsigned i = 0; i < 4; ++i)
        state.vertices[i].color = input[10 + i];
    Calls calls(input, output);
    sadx::X87Context fp(static_cast<std::uint16_t>(input[9]));
    try {
        sadx::menu_draw::draw_sized_texture(input[0], number(input[1]), number(input[2]),
                                            number(input[3]), number(input[4]), number(input[5]),
                                            state, static_cast<std::int32_t>(input[8]), calls, fp);
    } catch (const sadx::X87Error &) {
        output[29] = 1;
    }
    for (unsigned i = 0; i < 4; ++i) {
        const auto &v = state.vertices[i];
        const std::uint32_t words[]{bits(v.x), bits(v.y), bits(v.reciprocal_depth),
                                    bits(v.u), bits(v.v), v.color};
        std::copy_n(words, 6, output + i * 6);
    }
    output[30] = fp.control_word();
    output[31] = fp.exception_flags();
}
extern "C" void sadx_native_menu_vertex_colors(const std::uint32_t *input, std::uint32_t *output) {
    sadx::pvr::State state;
    sadx::pvr::initialize(state);
    state.base_color = input[4];
    sadx::menu_draw::set_vertex_colors({input[0], input[1], input[2], input[3]}, state);
    for (unsigned i = 0; i < 4; ++i)
        output[i] = state.vertices[i].color;
    output[4] = state.alpha_enabled;
    output[5] = state.base_color;
}

extern "C" std::int32_t sadx_native_menu_empty_text_width(const char *text) {
    sadx::font::State font;
    try {
        return sadx::menu_draw::text_width(text, font);
    } catch (const std::runtime_error &) {
        return -1;
    }
}
