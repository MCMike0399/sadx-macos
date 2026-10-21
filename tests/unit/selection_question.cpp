#include "game/selection_question.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace q = sadx::selection_question;
struct Calls final : q::Calls {
    std::array<std::int32_t, 3> &rotation;
    const sadx::NinjaVector &position;
    std::vector<unsigned> order;
    std::vector<std::uint16_t> angles;
    bool fail_draw = false, specular = false;
    unsigned pops = 0;
    std::int32_t observed_yaw = 0;
    Calls(std::array<std::int32_t, 3> &r, const sadx::NinjaVector &p) : rotation(r), position(p) {}
    void push_current_matrix() override { observed_yaw = rotation[1]; order.push_back(1); }
    void translate_current(const sadx::NinjaVector &p) override { assert(&p == &position); order.push_back(2); }
    void rotate_z(std::uint16_t a) override { order.push_back(3); angles.push_back(a); }
    void rotate_x(std::uint16_t a) override { order.push_back(4); angles.push_back(a); }
    void rotate_y(std::uint16_t a) override { order.push_back(5); angles.push_back(a); }
    void enable_specular() override { specular = true; order.push_back(6); }
    void select_menu_texture(std::uint32_t id) override { assert(id == 2); order.push_back(7); }
    void draw_object(std::uint32_t id) override {
        assert(id == 2942); order.push_back(8);
        if (fail_draw) throw std::runtime_error("actual renderer boundary missing");
    }
    void pop_matrix(std::uint32_t n) override { assert(n == 1); ++pops; order.push_back(9); }
};
int main() {
    const std::uint32_t object = 2942, texture = 2;
    const q::Assets assets{object, texture};
    const sadx::NinjaVector position{400, 0, -20};
    std::array<std::int32_t, 3> rotation{0, 16384, 0};
    const std::uint32_t one = 1, zero = 0;
    Calls normal(rotation, position);
    q::execute({rotation, position}, one, assets, normal);
    assert(rotation[1] == 17112 && normal.observed_yaw == 17112);
    assert((normal.order == std::vector<unsigned>{1, 2, 5, 6, 7, 8, 9}));
    assert(normal.specular && normal.pops == 1 && normal.angles[0] == 48424);
    // Every invocation remains the same exec; there is no guessed mode/init branch.
    Calls next(rotation, position);
    q::execute({rotation, position}, one, assets, next);
    assert(rotation[1] == 17840 && next.order == normal.order);
    rotation = {65536, 65536, 65536}; Calls high_bits(rotation, position);
    q::execute({rotation, position}, zero, assets, high_bits);
    assert((high_bits.order == std::vector<unsigned>{1, 2, 3, 4, 5, 6, 7, 8, 9}));
    assert((high_bits.angles == std::vector<std::uint16_t>{0, 0, 0}));
    rotation = {0, -728, 0}; Calls wrapped(rotation, position);
    q::execute({rotation, position}, one, assets, wrapped);
    assert(rotation[1] == 0 && wrapped.angles.empty() && wrapped.order.size() == 6);
    rotation = {0, 16384, 0}; Calls missing(rotation, position); missing.fail_draw = true;
    bool failed = false;
    try { q::execute({rotation, position}, one, assets, missing); }
    catch (const std::runtime_error &) { failed = true; }
    assert(failed && rotation[1] == 17112 && missing.specular && missing.pops == 0);
    assert((missing.order == std::vector<unsigned>{1, 2, 5, 6, 7, 8}));
    std::cout << "Question exec/update/render order, borrowed pose, full angle guards, repeat execution and explicit failed-render prefix pass.\n";
}
