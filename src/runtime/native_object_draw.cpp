#include "engine/texture_list_gate.hpp"
#include "game/game_mode.hpp"
#include "runtime/native_display.hpp"

namespace sadx::object_draw {
// NOTSA: one binding to the live session graph and existing display owners.
struct Context {
    native::Display &display;
    character_pack::ObjectState &objects;
    auto &matrices() { return *display.coordinates_; }
    auto &arithmetic() { return display.arithmetic_; }
    const auto *queued_list() const { return display.queued_texture_list_; }
};
namespace {
// STUB: SONIC 0x781770 njRotateXYZ numeric operation binding not yet verified
void rotate_default(Context &, std::array<std::int32_t, 3>) {
    throw flow::UnrecoveredGameFlowCall("native_object_rotate_xyz");
}
// STUB: SONIC 0x7819c0 njRotateZYX numeric operation binding not yet verified
void rotate_alternate(Context &, std::array<std::int32_t, 3>) {
    throw flow::UnrecoveredGameFlowCall("native_object_rotate_zyx");
}
} // namespace
const Object &object(Context &context, ObjectId id) { return context.objects.object(id); }
void push_matrix_ex(Context &context) { context.matrices().push_current_frame(); }
void translate_current(Context &context, const std::array<float, 3> &position) {
    context.matrices().translate_matrix(context.matrices().current_matrix(),
                                        {position[0], position[1], position[2]},
                                        context.arithmetic());
}
void rotate_current(Context &context, RotationOrder order, std::array<std::int32_t, 3> rotation) {
    if (order == RotationOrder::Default)
        rotate_default(context, rotation);
    else
        rotate_alternate(context, rotation);
}
void scale_current(Context &context, const std::array<float, 3> &scale) {
    context.matrices().scale_matrix(context.matrices().current_matrix(), scale[0], scale[1],
                                    scale[2], context.arithmetic());
}
void pop_matrix_ex(Context &context) { context.matrices().end_ex_frame(); }
bool texture_list_not_ready(Context &context) {
    return texture_selection::list_not_ready(context.queued_list());
}
} // namespace sadx::object_draw

namespace sadx::native {
void Display::draw_object_graph(character_pack::ResourceId id,
                                character_pack::ObjectState &objects) {
    object_draw::Context context{*this, objects};
    object_draw::State state{fog_.special_flags,    fog_.active,
                             queued_depth_,         frame_mode_.pending_logic_ticks,
                             simple_model_enabled_, queue_state_.values.lighting_palette};
    object_draw::draw(id, state, context);
}
} // namespace sadx::native
