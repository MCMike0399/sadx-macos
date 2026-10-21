#include "game/main_menu.hpp"
#include <cassert>
#include <cmath>
#include <stdexcept>

namespace m = sadx::main_menu;
struct Calls final : m::DrawCalls {
    unsigned blends = 0, colors = 0, draws = 0;
    void default_blending() override { ++blends; }
    void set_base_color(std::uint32_t color) override {
        assert(color == 1);
        ++colors;
    }
    void set_material(std::uint32_t color) override { assert(color == 4294967295U); }
    void select_texture(m::TextureList) override {}
    void draw_scaled(std::uint32_t, float, float, float, float, float) override { ++draws; }
};
int main() {
    m::Work work{};
    work.presentation.color = 1;
    float horizontal = 1, vertical = 1;
    std::uint32_t width = 640, height = 480, window = 99;
    m::DrawState view{horizontal, vertical, window, width, height};
    sadx::X87Context fp{127};
    Calls supported;
    m::draw_profiled(work, view, supported, fp);
    assert(supported.blends == 1 && supported.colors == 1 && supported.draws == 8 && window == 1);
    for (unsigned color : {0U, 1U}) {
        for (unsigned changed = 0; changed < 4; ++changed) {
            work.presentation.color = color;
            horizontal = vertical = 1;
            width = 640;
            height = 480;
            window = 99;
            if (changed == 0) width = 641;
            if (changed == 1) height = 481;
            if (changed == 2) horizontal = std::nextafter(1.0f, 2.0f);
            if (changed == 3) vertical = std::nextafter(1.0f, 2.0f);
            Calls calls;
            bool rejected = false;
            try {
                m::draw_profiled(work, view, calls, fp);
            } catch (const std::out_of_range &) {
                rejected = true;
            }
            assert(rejected == (color != 0));
            assert(calls.blends == color && calls.colors == color && calls.draws == 0);
            assert(window == 99);
        }
    }
}
