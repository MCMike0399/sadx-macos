#include "game/main_menu_content.hpp"
#include <cassert>
#include <vector>
namespace c = sadx::main_menu_content;
struct Calls final : c::Calls {
    std::vector<int> order;
    float &screen_x;
    c::Layout &layout;
    float x = 0, y = 0, sy = 0;
    Calls(float &scale, c::Layout &trial) : screen_x(scale), layout(trial) {}
    void select_common_texture() override {
        order.push_back(1);
        screen_x = 2;
        layout.offset_x = 20;
    }
    void set_material(std::uint32_t value) override {
        assert(value == 4294967295U);
        order.push_back(2);
    }
    bool optional_entry_available() override { order.push_back(3); return false; }
    void draw_scaled(std::uint32_t texture, float px, float py, float depth, float sx, float scale_y) override {
        assert(texture == 67108870U);
        assert(depth > 20);
        assert(sx > 0);
        x = px;
        y = py;
        sy = scale_y;
        order.push_back(4);
    }
};
int main() {
    float sx = 1, sy = 1;
    c::Layout trial{10, 10, .99225f, .8125f};
    Calls calls(sx, trial);
    sadx::menu_dialog::ContentArguments args{320, 240, 20, 1, 1, 0};
    sadx::X87Context fp{127};
    c::draw_main(args, {sx, sy, trial}, calls, fp);
    assert((calls.order == std::vector<int>{1, 2, 3, 4}));
    assert(calls.x == 650 && calls.y == 250);
    calls.order.clear();
    sx = 1;
    c::draw_trial(args, {sx, sy, trial}, calls, fp);
    assert((calls.order == std::vector<int>{1, 2, 4}));
    assert(calls.x == 660 && calls.y == 250 && calls.sy == .8125f);
}
