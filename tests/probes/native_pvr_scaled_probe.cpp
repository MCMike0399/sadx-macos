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
extern "C" void sadx_native_pvr_scaled(const std::uint32_t *in, std::uint32_t *out) {
    std::fill_n(out, 32, 0);
    sadx::pvr::State state;
    sadx::pvr::initialize(state);
    for (unsigned i = 0; i < 4; ++i) state.vertices[i].color = in[10 + i];
    Calls calls(in, out);
    sadx::X87Context fp(static_cast<std::uint16_t>(in[9]));
    sadx::pvr::draw_scaled_texture(in[0], number(in[1]), number(in[2]), number(in[3]),
                                  number(in[14]), number(in[15]), state,
                                  static_cast<std::int32_t>(in[8]), calls, fp);
    for (unsigned i = 0; i < 4; ++i) {
        const auto &v = state.vertices[i];
        const std::uint32_t words[]{bits(v.x), bits(v.y), bits(v.reciprocal_depth),
                                   bits(v.u), bits(v.v), v.color};
        std::copy_n(words, 6, out + i * 6);
    }
    out[30] = fp.control_word(); out[31] = fp.exception_flags();
}
