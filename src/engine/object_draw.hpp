#pragma once

#include "engine/character_pack.hpp"

namespace sadx::object_draw {
using ObjectId = character_pack::ResourceId;
using ModelId = character_pack::ResourceId;
using Object = character_pack::ObjectResource;
using Model = character_pack::ModelResource;

// NJD_EVAL_* in ninja.h. Higher evaluation bits do not affect this traversal.
enum class ObjectFlags : std::uint32_t {
    UnitPosition = 1,
    UnitAngle = 2,
    UnitScale = 4,
    Hide = 8,
    Break = 16,
    AlternateRotation = 32,
    UnitTransform = 7,
};
enum class RotationOrder { Default, Alternate };

// NOTSA: borrows canonical renderer and frame owners; no snapshot restoration.
struct State {
    std::uint8_t &fog_special_flags;
    const std::uint8_t &fog_active;
    float &queued_depth;
    const std::int32_t &pending_logic_ticks;
    const std::uint8_t &simple_draw_enabled;
    const std::int32_t &lighting_palette;
};

// NOTSA: defined by the integration binding, borrowing its existing graph,
// NinjaMatrixPrimitives, FP context and renderer. This module owns none of them.
struct Context;

// NOTSA: stable reference into the one mutable graph. Missing IDs fail at the
// asset boundary. Model zero denotes a known non-drawable imported reference.
const Object &object(Context &context, ObjectId id);
// NOTSA: bind the existing NinjaMatrixPrimitives operations, preserving their
// capacity semantics. Default/alternate rotation denote the source operations,
// not an inferred Euler convention. See PACKET.md for the exact binding contract.
void push_matrix_ex(Context &context);
void translate_current(Context &context, const std::array<float, 3> &position);
// Source passes the three angles by value, unlike translation/scale pointers.
void rotate_current(Context &context, RotationOrder order, std::array<std::int32_t, 3> rotation);
void scale_current(Context &context, const std::array<float, 3> &scale);
void pop_matrix_ex(Context &context);
// NOTSA: delegate the existing texture-list gate and propagate missing facts.
bool texture_list_not_ready(Context &context);

// STUB: SONIC 0x407b70 late_DrawObject actual queued rendering required
void draw_late_object(Context &context, ObjectId id, std::uint32_t flags);
// STUB: SONIC 0x77ef70 njDrawModel actual model rendering required
void draw_model(Context &context, ModelId id);

// FUNCTION: SONIC 0x401430 dsSimpleDrawModel
std::int32_t draw_simple_model(ModelId model, Context &context);
// FUNCTION: SONIC 0x4034b0 DrawObject
void draw_objects(ObjectId root, State state, Context &context);
// FUNCTION: SONIC 0x408530 dsDrawObject
void draw(ObjectId root, State state, Context &context);
} // namespace sadx::object_draw
