#include "object_draw_compat.hpp"

namespace sadx::object_draw {
struct Context {
    Calls &calls;
};
const Object &object(Context &context, ObjectId id) { return context.calls.object(id); }
void push_matrix_ex(Context &context) { context.calls.push_matrix_ex(); }
void translate_current(Context &context, const std::array<float, 3> &position) {
    context.calls.translate_current(position);
}
void rotate_current(Context &context, RotationOrder order, std::array<std::int32_t, 3> rotation) {
    context.calls.rotate_current(order, rotation);
}
void scale_current(Context &context, const std::array<float, 3> &scale) {
    context.calls.scale_current(scale);
}
void pop_matrix_ex(Context &context) { context.calls.pop_matrix_ex(); }
bool texture_list_not_ready(Context &context) { return context.calls.texture_list_not_ready(); }
void draw_late_object(Context &context, ObjectId id, std::uint32_t flags) {
    context.calls.draw_late_object(id, flags);
}
void draw_model(Context &context, ModelId id) { context.calls.draw_model(id); }
std::int32_t draw_simple_model(ModelId model, Calls &calls) {
    Context context{calls};
    return draw_simple_model(model, context);
}
void draw_objects(ObjectId root, State state, Calls &calls) {
    Context context{calls};
    draw_objects(root, state, context);
}
void draw(ObjectId root, State state, Calls &calls) {
    Context context{calls};
    draw(root, state, context);
}
} // namespace sadx::object_draw
