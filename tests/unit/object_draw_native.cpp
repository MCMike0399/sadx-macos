#include "engine/ninja_matrix.hpp"
#include "engine/object_draw.hpp"
#include <cassert>
#include <map>
#include <stdexcept>
#include <vector>

namespace sadx::object_draw {
// Test binding: no runtime, asset pack, executable or reference compatibility.
struct Context {
    std::map<ObjectId, Object> graph;
    NinjaMatrixPrimitives &matrices;
    X87Context &fp;
    std::vector<ModelId> models{};
    ObjectId current = 0;
    bool change_after_push = false;
    bool missing_renderer = false;
    bool missing_late = false;
    bool missing_gate = false;
    unsigned gates = 0;
};
const Object &object(Context &context, ObjectId id) {
    context.current = id;
    return context.graph.at(id);
}
void push_matrix_ex(Context &context) {
    context.matrices.push_current_frame();
    if (context.change_after_push) {
        auto &node = context.graph.at(context.current);
        node.evaluation_flags = 31; // flags must remain cached by traversal
        node.model = 42;
        node.child = 2;
    }
}
void translate_current(Context &context, const std::array<float, 3> &position) {
    context.matrices.translate_matrix(context.matrices.current_matrix(),
                                      {position[0], position[1], position[2]}, context.fp);
}
void rotate_current(Context &context, RotationOrder, std::array<std::int32_t, 3> rotation) {
    const auto argument = rotation;
    context.graph.at(context.current).rotation[0] ^= 1;
    assert(rotation == argument); // source angles were copied before the call
    throw std::runtime_error("rotation binding outside this native test");
}
void scale_current(Context &context, const std::array<float, 3> &scale) {
    context.matrices.scale_matrix(context.matrices.current_matrix(), scale[0], scale[1], scale[2],
                                  context.fp);
}
void pop_matrix_ex(Context &context) {
    context.matrices.end_ex_frame();
    // Must be read after the pop too, not only after child recursion.
    context.graph.at(1).sibling = 3;
}
bool texture_list_not_ready(Context &context) {
    ++context.gates;
    if (context.missing_gate)
        throw std::runtime_error("texture gate unavailable");
    return false;
}
void draw_model(Context &context, ModelId model) {
    if (context.missing_renderer)
        throw std::runtime_error("njDrawModel unavailable");
    context.models.push_back(model);
    if (model == 42) {
        assert(context.matrices.current_matrix()[12] == 1);
        assert(context.matrices.current_matrix()[13] == 2);
        assert(context.matrices.current_matrix()[14] == 3);
    }
}
void draw_late_object(Context &context, ObjectId, std::uint32_t) {
    if (context.missing_late)
        throw std::runtime_error("late_DrawObject unavailable");
    assert(false && "late rendering is outside this native test");
}
} // namespace sadx::object_draw

int main() {
    namespace d = sadx::object_draw;
    const sadx::NinjaMatrix unit{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    sadx::NinjaMatrixStack stack({unit, unit, unit}, 1, 0);
    // Rotation remains a loud boundary; only existing push/translate/scale/pop are bound.
    sadx::NinjaMatrixPrimitives matrices({}, unit, stack);
    sadx::X87Context fp(127);
    d::Context context{{}, matrices, fp};
    context.graph = {{1, {2, 1, {1, 2, 3}, {}, {2, 3, 4}, 0, 0}},
                     {2, {23, 2, {}, {}, {}, 0, 0}},
                     {3, {23, 3, {}, {}, {}, 0, 0}}};
    context.change_after_push = true;
    std::uint8_t fog = 0, active = 0, simple = 1;
    float depth = 0;
    std::int32_t pending = 0, palette = 2;
    d::State state{fog, active, depth, pending, simple, palette};
    d::draw(1, state, context);
    assert((context.models == std::vector<d::ModelId>{42, 2, 3}));
    assert(stack.depth() == 1 && stack.current() == unit);

    context.change_after_push = false;
    context.graph.at(1).evaluation_flags = 2;
    context.missing_renderer = true;
    bool failed = false;
    try {
        d::draw(1, state, context);
    } catch (const std::runtime_error &) {
        failed = true;
    }
    assert(failed && stack.depth() == 2); // no invented rollback at the seam
    matrices.end_ex_frame();

    context.missing_gate = true;
    failed = false;
    try {
        d::draw(0, state, context);
    } catch (const std::runtime_error &) {
        failed = true;
    }
    assert(failed && stack.depth() == 1);

    fog = 3;
    context.missing_late = true;
    const auto previous_gates = context.gates;
    failed = false;
    try {
        d::draw(1, state, context);
    } catch (const std::runtime_error &) {
        failed = true;
    }
    assert(failed && fog == 1 && depth == -47952.0f);
    assert(context.gates == previous_gates && stack.depth() == 1);

    fog = 0;
    context.missing_gate = false;
    context.graph.at(1).evaluation_flags = 5;
    context.graph.at(1).rotation = {10, 20, 30};
    failed = false;
    try {
        d::draw(1, state, context);
    } catch (const std::runtime_error &) {
        failed = true;
    }
    assert(failed && stack.depth() == 2 && context.graph.at(1).rotation[0] == 11);
}
