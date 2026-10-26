#include "engine/object_draw.hpp"
#include <cassert>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
namespace d = sadx::object_draw;
d::Object node(std::uint32_t flags, std::uint32_t model, std::uint32_t child = 0,
               std::uint32_t sibling = 0) {
    return {flags, model, {1, 2, 3}, {10, 20, 30}, {1, 1, 1}, child, sibling};
}
struct Calls final : d::Calls {
    std::map<d::ObjectId, d::Object> graph;
    std::vector<unsigned> order, models;
    std::uint8_t fog = 0, active = 0, simple = 1;
    float depth = 0;
    std::int32_t pending = 0, palette = 2;
    bool late_mutates = false, fail_model = false, fail_gate = false;
    unsigned gates = 0, pushes = 0, pops = 0;
    d::State state() { return {fog, active, depth, pending, simple, palette}; }
    const d::Object &object(d::ObjectId id) override { return graph.at(id); }
    void push_matrix_ex() override { ++pushes; order.push_back(3); }
    void translate_current(const std::array<float, 3> &) override { order.push_back(4); }
    void rotate_current(d::RotationOrder kind, const std::array<std::int32_t, 3> &) override {
        order.push_back(kind == d::RotationOrder::Default ? 5 : 6);
    }
    void scale_current(const std::array<float, 3> &) override { order.push_back(7); }
    void pop_matrix_ex() override { ++pops; order.push_back(8); }
    bool texture_list_not_ready() override {
        ++gates; order.push_back(2);
        if (fail_gate) throw std::runtime_error("actual texture-list boundary missing");
        return false;
    }
    void draw_late_object(d::ObjectId, std::uint32_t flags) override {
        assert(flags == 1 && (fog & 2U) == 0 && depth == -47952.0f); order.push_back(1);
        if (late_mutates) { fog = 164; depth = -7; pending = 1; }
    }
    void draw_model(d::ModelId model) override {
        order.push_back(9); models.push_back(model);
        if (model == 2 && graph.count(1)) graph.at(1).sibling = 4;
        if (fail_model) throw std::runtime_error("actual model renderer missing");
    }
};
int main() {
    Calls question; question.graph.emplace(1, node(23, 2943));
    d::draw(1, question.state(), question);
    assert((question.order == std::vector<unsigned>{2, 9}));
    assert(question.pushes == 0 && question.pops == 0 && question.models[0] == 2943);
    Calls graph; graph.graph = {{1, node(0, 1, 2, 3)}, {2, node(23, 2)},
                                {3, node(23, 3)}, {4, node(23, 4)}};
    d::draw(1, graph.state(), graph);
    assert((graph.models == std::vector<unsigned>{1, 2, 4}));
    assert((graph.order == std::vector<unsigned>{2, 3, 4, 5, 7, 9, 9, 8, 9}));
    assert(graph.pushes == 1 && graph.pops == 1);
    Calls late; late.fog = 3; late.late_mutates = true;
    d::draw(0, late.state(), late);
    assert(late.fog == 166 && late.depth == -7 && late.gates == 0);
    assert((late.order == std::vector<unsigned>{1}));
    Calls empty; d::draw(0, empty.state(), empty); assert(empty.gates == 1);
    Calls failed; failed.graph.emplace(1, node(0, 1)); failed.fail_model = true;
    bool missing = false;
    try { d::draw(1, failed.state(), failed); } catch (const std::runtime_error &) { missing = true; }
    assert(missing && failed.pushes == 1 && failed.pops == 0);
    Calls unknown; unknown.fail_gate = true; missing = false;
    try { d::draw(0, unknown.state(), unknown); } catch (const std::runtime_error &) { missing = true; }
    assert(missing && unknown.models.empty());
    Calls simple; assert(d::draw_simple_model(123, simple) == 0 && simple.models[0] == 123);
    std::cout << "Borrowed live object links, exact Question no-transform path, late state reloads, real gate boundary and loud renderer prefix pass.\n";
}
