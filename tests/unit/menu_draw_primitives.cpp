#include "engine/menu_draw_primitives.hpp"
#include <cassert>
#include <limits>
namespace {
struct Calls final : sadx::menu_draw::Calls {
    unsigned lookups = 0, draws = 0;
    sadx::pvr::TextureInfo texture(std::uint32_t) override {
        ++lookups;
        return {55, 0, 257, 255};
    }
    void draw_texture(const std::array<sadx::pvr::Vertex, 4> &, std::uint32_t id,
                      std::uint32_t alpha) override {
        assert(id == 55 && alpha == 1);
        ++draws;
    }
    void select_standard_texture(sadx::menu_draw::StandardTexture) override {}
    void set_blend(std::uint32_t, std::uint32_t) override {}
    void reset_material() override {}
    void set_depth_compare(std::uint32_t) override {}
    void set_depth_write(std::uint32_t) override {}
    void restore_depth_compare() override {}
    void draw_colored_polygon(const std::array<sadx::pvr::Point, 4> &,
                              const std::array<std::uint32_t, 4> &, float, std::uint32_t) override {
    }
    std::uint32_t texture_global_index(const sadx::texture_lifetime::List &,
                                       std::uint32_t) override {
        return 55;
    }
    void draw_polygon(const std::array<sadx::pvr::Point, 4> &, std::uint32_t, float,
                      std::uint32_t) override {}
    void queue_box(float, float, float, float, float, std::uint32_t) override {}
};
} // namespace
int main() {
    using namespace sadx;
    pvr::State state;
    pvr::initialize(state);
    state.base_color = 2151686160U;
    menu_draw::set_vertex_colors({4286595104U, 4294967295U, 0, 4294967295U}, state);
    assert(state.vertices[0].color == 2149582850U && state.alpha_enabled == 1);
    Calls calls;
    X87Context fp(895);
    menu_draw::draw_sized_texture(3288334336U, 100, 200, 2, 3.25f, 7.5f, state, 0, calls, fp);
    assert(calls.lookups == 1 && calls.draws == 1);
    assert(state.vertices[0].x == 98.375f && state.vertices[0].y == 196.25f);
    assert(state.vertices[3].x == 101.625f && state.vertices[3].y == 203.75f);
    assert(state.vertices[0].u == 1 && state.vertices[0].v == 1 &&
           state.vertices[0].reciprocal_depth == 0.5f);
    state = {};
    pvr::initialize(state);
    calls = {};
    X87Context overflow(119);
    bool failed = false;
    try {
        menu_draw::draw_sized_texture(67108864U, 100, 200, std::numeric_limits<float>::denorm_min(),
                                      3.25f, 7.5f, state, 0, calls, overflow);
    } catch (const X87Error &error) {
        failed = true;
        assert((error.exceptions() & 8) != 0);
    }
    assert(failed && calls.lookups == 1 && calls.draws == 0);
    assert(state.vertices[1].y == 203.75f && state.vertices[2].x == 101.625f &&
           state.vertices[3].x == 101.625f);
    assert(state.vertices[0].x == 0 && state.vertices[0].y == 0 && state.vertices[1].x == 0);
    font::State font;
    assert(menu_draw::text_width("", font) == 0);
    assert(menu_draw::text_width(std::string_view("\0A", 2), font) == 0);
    std::array<std::uint8_t, 128> widths{};
    widths['A'] = 8;
    font.current.proportions = &widths;
    assert(menu_draw::text_width("A A", font) == 22);
    assert(menu_draw::text_width(std::string_view("A\0 A", 4), font) == 9);
}
