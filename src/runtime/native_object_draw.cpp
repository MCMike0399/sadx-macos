#include "engine/animated_model.hpp"
#include "engine/ninja_matrix_composition.hpp"
#include "engine/texture_list_gate.hpp"
#include "game/game_mode.hpp"
#include "runtime/native_display.hpp"
#include "runtime/object_draw_context.hpp"

namespace sadx::object_draw {
const Object &object(Context &context, ObjectId id) {
    return context.environment.scene.objects.object(id);
}
void push_matrix_ex(Context &context) { context.matrices().push_current_frame(); }
void translate_current(Context &context, const std::array<float, 3> &position) {
    translate_ninja_matrix(context.matrices().current_matrix(), position[0], position[1],
                           position[2], context.arithmetic());
}
void rotate_current(Context &context, RotationOrder order, std::array<std::int32_t, 3> rotation) {
    if (order == RotationOrder::Default)
        rotate_ninja_xyz(context.matrices(), context.matrices().current_matrix(), rotation,
                         context.arithmetic());
    else
        rotate_ninja_zyx(context.matrices(), context.matrices().current_matrix(), rotation,
                         context.arithmetic());
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
                                const ModelDrawEnvironment &environment) {
    object_draw::Context context{*this, environment};
    object_draw::State state{fog_.special_flags,    fog_.active,
                             queued_depth_,         frame_mode_.pending_logic_ticks,
                             simple_model_enabled_, queue_state_.values.lighting_palette};
    object_draw::draw(id, state, context);
}
} // namespace sadx::native

namespace sadx::animated_model {
fidelity::Context &arithmetic(object_draw::Context &context) { return context.arithmetic(); }
void object_callback(object_draw::Context &context, ObjectId id) {
    if (!context.animated_callback)
        throw flow::UnrecoveredGameFlowCall("native_animated_model_object_callback");
    context.animated_callback(id);
}
bool check_view(object_draw::Context &context, object_draw::ModelId id, float) {
    if (context.environment.pack.model(id).radius == 0)
        return true;
    throw flow::UnrecoveredGameFlowCall("native_animated_model_nonzero_clip_scale_dsCheckViewV");
}
} // namespace sadx::animated_model
