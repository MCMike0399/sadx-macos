#pragma once
#include "cpu/fidelity_float.hpp"
#include "engine/object_draw.hpp"

namespace sadx::animated_model {
using ObjectId = object_draw::ObjectId;
enum class ModelCallback { None, DrawModel };
enum class ObjectCallback { None, Gamma };
// NOTSA: borrowed view of the existing imported or generated animation runtime.
struct Motion {
    virtual ~Motion() = default;
    virtual std::uint32_t frame_count() = 0;
    virtual std::uint16_t type() = 0;
    virtual std::uint16_t interpolation() = 0;
    virtual bool has_keys(unsigned node, unsigned channel) { return key_count(node, channel) != 0; }
    virtual std::uint32_t key_count(unsigned node, unsigned channel) = 0;
    virtual std::uint32_t key_word(unsigned node, unsigned channel, unsigned key,
                                   unsigned word) = 0;
};
struct State {
    // GLOBAL: SONIC 0x3abd95c AnimatedModelCallback size=4
    ModelCallback model_callback = ModelCallback::None;
    // GLOBAL: SONIC 0x3abd9a4 AnimatedDrawFlags size=4
    std::uint32_t flags = 0;
    // GLOBAL: SONIC 0x3ab9930 CallbackModelScale size=4
    float clip_scale = 0;
    // GLOBAL: SONIC 0x3ab991c AnimatedMotionFrame size=4
    float frame = 0;
    // GLOBAL: SONIC 0x3ab992c AnimatedMotionAuxiliaryIndex size=4
    unsigned motion_auxiliary_index = 0;
    // NOTSA: resource node ordinal replaces the MDATA cursor's native pointer.
    unsigned node = 0;
    object_draw::RotationOrder rotation_order = object_draw::RotationOrder::Default;
    // GLOBAL: SONIC 0x3ab9924 AnimatedMotionChannel size=4
    unsigned channel = 0;
    // GLOBAL: SONIC 0x3ab9894 AnimatedMotionType size=2
    std::uint16_t motion_type = 0;
    // GLOBAL: SONIC 0x3ab9904 ObjectDrawFuncs_Index size=2
    std::uint16_t interpolation = 0;
    // GLOBAL: SONIC 0x3ab9908 AnimatedObjectCallback size=4
    ObjectCallback object_callback = ObjectCallback::None;
};
// NOTSA: numeric operations borrow the same maintained display arithmetic owner.
fidelity::Context &arithmetic(object_draw::Context &);
// NOTSA: dispatches the animated callback slot, distinct from njSetObjectCallback.
void object_callback(object_draw::Context &, ObjectId);
// STUB: SONIC 0x403330 dsCheckViewV nonzero callback clip scale not composed
bool check_view(object_draw::Context &, object_draw::ModelId, float);

// NOTSA: adapters defer action payload access until the source loop guard passes.
bool drawing_enabled(object_draw::State);
// FUNCTION: SONIC 0x4051d0 SetupAnimatedMotion
void setup_motion(Motion *, float, State &, fidelity::Context &);
// STUB: SONIC 0x405260 SetObjectDrawFunctions MDATA2 layout only; other index layouts unrecovered
void select_transforms(ObjectId, State &, object_draw::Context &);
// FUNCTION: SONIC 0x404fd0 TransformAnimatedObjectMdata2
void transform_mdata2(ObjectId, Motion &, State &, object_draw::Context &);
// FUNCTION: SONIC 0x4050c0 ProcessAnimatedModelNode__
void process_nodes(ObjectId, Motion &, State &, object_draw::State, object_draw::Context &);
// FUNCTION: SONIC 0x4053d0 DrawAction
void draw_action(ObjectId, Motion *, float, std::uint32_t, State &, object_draw::State,
                 object_draw::Context &);
// FUNCTION: SONIC 0x407020 ds_ActionClipNoScale
void action_clip_no_scale(ObjectId, Motion *, float, State &, object_draw::State,
                          object_draw::Context &);
} // namespace sadx::animated_model
