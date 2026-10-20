#include "engine/pvr_draw.hpp"
#include "engine/pvr_device.hpp"
#include <algorithm>
#include <cstring>
namespace {
float number(std::uint32_t v) { float f; std::memcpy(&f, &v, 4); return f; }
std::uint32_t bits(float f) { std::uint32_t v; std::memcpy(&v, &f, 4); return v; }
struct Calls final : sadx::pvr::Calls {
    const std::uint32_t *in;
    std::uint32_t *out;
    Calls(const std::uint32_t *i, std::uint32_t *o) : in(i), out(o) {}
    sadx::pvr::TextureInfo texture(std::uint32_t index) override {
        out[28] = index;
        return {in[6], in[7], in[4], in[5]};
    }
    void draw_texture(const std::array<sadx::pvr::Vertex, 4> &, std::uint32_t index,
                      std::uint32_t alpha) override {
        out[24] = 1; out[25] = index; out[26] = alpha;
    }
    void draw_polygon(const std::array<sadx::pvr::Point, 4> &p, std::uint32_t c,
                      float priority, std::uint32_t flags) override {
        for (unsigned i = 0; i < 4; ++i) {
            out[i * 2] = bits(p[i].x); out[i * 2 + 1] = bits(p[i].y);
        }
        out[8] = c; out[9] = bits(priority); out[10] = flags; out[11] = 1;
    }
    void queue_box(float, float, float, float, float, std::uint32_t) override {
        out[12] += 1;
    }
};
}
extern "C" void sadx_native_pvr_draw(const std::uint32_t *in, std::uint32_t *out) {
    std::fill_n(out, 32, 0);
    sadx::pvr::State state;
    sadx::pvr::initialize(state);
    for (unsigned i = 0; i < 4; ++i) state.vertices[i].color = in[10 + i];
    Calls calls(in, out);
    sadx::X87Context fp(static_cast<std::uint16_t>(in[9]));
    sadx::pvr::draw_texture(in[0], number(in[1]), number(in[2]), number(in[3]),
                           state, static_cast<std::int32_t>(in[8]), calls, fp);
    for (unsigned i = 0; i < 4; ++i) {
        const auto &v = state.vertices[i];
        const std::uint32_t words[]{bits(v.x), bits(v.y), bits(v.reciprocal_depth),
                                   bits(v.u), bits(v.v), v.color};
        std::copy_n(words, 6, out + i * 6);
    }
    out[30] = fp.control_word(); out[31] = fp.exception_flags();
}
extern "C" void sadx_native_pvr_box(const std::uint32_t *in, std::uint32_t *out) {
    std::fill_n(out, 32, 0);
    auto disabled = static_cast<std::int32_t>(in[6]);
    auto fog = static_cast<std::uint8_t>(in[7]), enabled = static_cast<std::uint8_t>(in[8]);
    float depth = 0;
    Calls calls(in, out);
    sadx::X87Context fp(static_cast<std::uint16_t>(in[9]));
    sadx::pvr::draw_box_fill_2d(number(in[0]), number(in[1]), number(in[2]), number(in[3]),
                              number(in[4]), in[5], disabled, fog, enabled, depth, calls, fp);
    out[13] = fog; out[14] = bits(depth);
    out[30] = fp.control_word(); out[31] = fp.exception_flags();
}
extern "C" void sadx_native_pvr_colors(const std::uint32_t *in, std::uint32_t *out) {
    sadx::pvr::State state;
    sadx::pvr::initialize(state);
    state.base_color = in[2];
    sadx::pvr::set_material(in[0], state);
    sadx::pvr::set_base_color(in[1], state);
    for (unsigned i = 0; i < 4; ++i) out[i] = state.vertices[i].color;
    out[4] = state.base_color;
    out[5] = state.alpha_enabled;
}
extern "C" void sadx_native_pvr_defaults(std::uint32_t *out) {
    sadx::graphics::Draw state;
    sadx::pvr::set_default_render_state(state);
    sadx::pvr::set_default_texture_stage_state(state);
    for (const auto &entry : state.render_states) {
        *out++ = entry.first;
        *out++ = entry.second;
    }
    for (const auto &entry : state.stage0_states) {
        *out++ = entry.first;
        *out++ = entry.second;
    }
}
