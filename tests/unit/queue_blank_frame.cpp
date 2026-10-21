#include "engine/queue_blank_frame.hpp"
#include <cassert>
#include <stdexcept>
#include <vector>
namespace {
struct Calls final : sadx::pvr::Calls {
    std::vector<unsigned> order;
    float &h, &v, &offset;
    std::int32_t &disabled;
    std::uint8_t &special;
    unsigned &default_depth;
    explicit Calls(float &x, float &y, float &z, std::int32_t &d, std::uint8_t &s, unsigned &depth)
        : h(x), v(y), offset(z), disabled(d), special(s), default_depth(depth) {}
    sadx::pvr::TextureInfo texture(std::uint32_t) override {
        throw std::logic_error("unexpected texture lookup");
    }
    void draw_texture(const std::array<sadx::pvr::Vertex, 4> &, std::uint32_t,
                      std::uint32_t) override {
        throw std::logic_error("unexpected textured primitive");
    }
    void draw_polygon(const std::array<sadx::pvr::Point, 4> &p, std::uint32_t color, float z,
                      std::uint32_t flags) override {
        order.push_back(1);
        assert(p[0].x == 0 && p[0].y == 0 && p[1].x == 0 && p[1].y == 480 && p[2].x == 640 &&
               p[2].y == 0 && p[3].x == 640 && p[3].y == 480);
        assert(color == 4278190080U && z == -1 && flags == 34);
        default_depth = 6;
    }
    void queue_box(float l, float t, float r, float b, float z, std::uint32_t color) override {
        order.push_back(2);
        assert(l == 0 && t == 0 && r == 640 && b == 480 && z == -1 && color == 4278190080U);
        assert(special == 1 && offset == -47952);
        special = 5;
        disabled = 0;
        h = 2;
        v = 2;
    }
};
} // namespace
int main() {
    using namespace sadx;
    float h = 1, v = 1, offset = 17;
    std::int32_t disabled = 0;
    std::uint8_t special = 0, fog = 0;
    unsigned depth = 1;
    Calls calls(h, v, offset, disabled, special, depth);
    X87Context fp(127);
    queue::BlankFrameView view{h, v, disabled, special, fog, offset};
    queue::draw_blank_frame(
        view, calls,
        [&] {
            calls.order.push_back(3);
            assert(depth == 6);
        },
        fp);
    assert((calls.order == std::vector<unsigned>{1, 3}));
    calls.order.clear();
    disabled = 1;
    queue::draw_blank_frame(view, calls, [&] { calls.order.push_back(3); }, fp);
    assert((calls.order == std::vector<unsigned>{3}));
    calls.order.clear();
    special = 3;
    queue::draw_blank_frame(view, calls, [&] { calls.order.push_back(3); }, fp);
    assert((calls.order == std::vector<unsigned>{2, 1, 3}) && special == 7 && disabled == 0);
    h = v = 1;
    special = 0;
    calls.order.clear();
    bool failed = false;
    try {
        queue::draw_blank_frame(view, calls, {}, fp);
    } catch (const queue::BlankFrameError &error) {
        failed = error.boundary == queue::BlankFrameBoundary::DefaultDepthRestore;
    }
    assert(failed && (calls.order == std::vector<unsigned>{1}));
}
