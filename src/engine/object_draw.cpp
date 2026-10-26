#include "engine/object_draw.hpp"

namespace sadx::object_draw {
namespace {
enum class FogFlags : std::uint8_t { Enabled = 1, Emulated = 2 };
constexpr float kLateObjectDepth = -47952.0f;
constexpr std::uint32_t kLateObjectFlags = 1;

// NOTSA: typed interpretation of the imported object's NJD_EVAL flags.
bool has(ObjectFlags flags, ObjectFlags mask) {
    return (static_cast<std::uint32_t>(flags) & static_cast<std::uint32_t>(mask)) ==
           static_cast<std::uint32_t>(mask);
}
} // namespace

std::int32_t draw_simple_model(ModelId model, Context &context) {
    draw_model(context, model);
    return 0;
}

void draw_objects(ObjectId root, State state, Context &context) {
    while (root != 0) {
        const auto &node = object(context, root);
        // Source retains flags across calls, but reloads model and links later.
        const auto flags = static_cast<ObjectFlags>(node.evaluation_flags);
        const bool transformed = !has(flags, ObjectFlags::UnitTransform);
        if (transformed) {
            push_matrix_ex(context);
            if (!has(flags, ObjectFlags::UnitPosition))
                translate_current(context, node.position);
            if (!has(flags, ObjectFlags::UnitAngle))
                rotate_current(context,
                               has(flags, ObjectFlags::AlternateRotation) ? RotationOrder::Alternate
                                                                          : RotationOrder::Default,
                               node.rotation);
            if (!has(flags, ObjectFlags::UnitScale))
                scale_current(context, node.scale);
        }
        if (!has(flags, ObjectFlags::Hide) && node.model != 0) {
            if (state.simple_draw_enabled != 0 && state.lighting_palette <= 0)
                draw_simple_model(node.model, context);
            else
                draw_model(context, node.model);
        }
        if (!has(flags, ObjectFlags::Break))
            draw_objects(node.child, state, context);
        if (transformed)
            pop_matrix_ex(context);
        // A child or model callback can replace this same object's sibling.
        root = node.sibling;
    }
}

void draw(ObjectId root, State state, Context &context) {
    constexpr auto enabled = static_cast<std::uint8_t>(FogFlags::Enabled);
    constexpr auto emulated = static_cast<std::uint8_t>(FogFlags::Emulated);
    constexpr auto both = enabled | emulated;
    if ((state.fog_special_flags & both) == both && state.fog_active == 0) {
        state.fog_special_flags &= static_cast<std::uint8_t>(~emulated);
        state.queued_depth = kLateObjectDepth;
        draw_late_object(context, root, kLateObjectFlags);
        // Reload after the callback; its writes survive this restoration.
        state.fog_special_flags |= emulated;
    }
    if (state.pending_logic_ticks == 0 && !texture_list_not_ready(context))
        draw_objects(root, state, context);
}
} // namespace sadx::object_draw
