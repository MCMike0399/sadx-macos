#include "engine/animated_model.hpp"
#include <cstring>
#include <stdexcept>

namespace sadx::animated_model {
namespace {
using fidelity::ExactFloat;
// NOTSA: imported key words retain the original float/integer bit patterns.
template <class T> T value(std::uint32_t word) {
    T result;
    std::memcpy(&result, &word, sizeof(result));
    return result;
}
// NOTSA: unsigned source conversion uses signed FILD plus the correction literal.
ExactFloat unsigned_value(fidelity::Context &fp, std::uint32_t word) {
    auto result = ExactFloat::integer(fp, value<std::int32_t>(word));
    if (value<std::int32_t>(word) < 0)
        result = result + 4294967296.0f;
    return result;
}

// NOTSA: shared source key interval selection before either linear sampler.
std::array<std::uint32_t, 2> interval(Motion &motion, State &state, unsigned channel,
                                      fidelity::Context &fp) {
    const auto count = motion.key_count(state.node, channel);
    if (count == 0)
        throw std::runtime_error("animated_model_zero_key_count");
    const auto frame = static_cast<std::uint32_t>(ExactFloat(fp, state.frame).truncated_integer());
    unsigned first = 0;
    while (first + 1 < count && motion.key_word(state.node, channel, first + 1, 0) <= frame)
        ++first;
    return {first, (first + 1) % count};
}
// NOTSA: both source samplers retain the fraction, after spilling the denominator.
ExactFloat fraction(Motion &motion, State &state, unsigned channel,
                    const std::array<std::uint32_t, 2> &keys, fidelity::Context &fp) {
    const auto first = motion.key_word(state.node, channel, keys[0], 0);
    const auto second = motion.key_word(state.node, channel, keys[1], 0);
    const auto difference = second - first;
    const auto denominator = unsigned_value(fp, difference).rounded();
    const auto delta = ExactFloat(fp, state.frame) - unsigned_value(fp, first);
    return delta / (value<std::int32_t>(difference) > 0 ? denominator : 1.0f);
}
// FUNCTION: SONIC 0x404b60 LinearMotionVector
std::array<float, 3> sample_vector(Motion &motion, State &state, unsigned channel,
                                   fidelity::Context &fp) {
    const auto keys = interval(motion, state, channel, fp);
    const auto next_weight = fraction(motion, state, channel, keys, fp);
    const auto first_weight = ExactFloat(fp, 1.0f) - next_weight;
    std::array<float, 3> result{};
    for (unsigned axis = 0; axis < 3; ++axis) {
        const auto first =
            first_weight * value<float>(motion.key_word(state.node, channel, keys[0], axis + 1));
        const auto second =
            next_weight * value<float>(motion.key_word(state.node, channel, keys[1], axis + 1));
        result[axis] = (second + first).rounded();
    }
    return result;
}
// FUNCTION: SONIC 0x404c10 LinearMotionAngles
std::array<std::int32_t, 3> sample_angles(Motion &motion, State &state, unsigned channel,
                                          fidelity::Context &fp) {
    const auto keys = interval(motion, state, channel, fp);
    const auto next_weight = fraction(motion, state, channel, keys, fp);
    const auto first_weight = ExactFloat(fp, 1.0f) - next_weight;
    std::array<std::int32_t, 3> result{};
    for (unsigned axis = 0; axis < 3; ++axis) {
        const auto first =
            ExactFloat::integer(
                fp, value<std::int32_t>(motion.key_word(state.node, channel, keys[0], axis + 1))) *
            first_weight;
        const auto second =
            ExactFloat::integer(
                fp, value<std::int32_t>(motion.key_word(state.node, channel, keys[1], axis + 1))) *
            next_weight;
        result[axis] = static_cast<std::int32_t>((first + second).truncated_integer());
    }
    return result;
}
// NOTSA: dispatch selected once from the motion type and root rotation flag.
void transform_channel(ObjectId id, unsigned component, Motion &motion, State &state,
                       object_draw::Context &context) {
    const auto &node = object_draw::object(context, id);
    const auto bit = 1U << component;
    const bool sampled = (state.motion_type & bit) != 0;
    const auto channel = state.channel;
    auto &fp = arithmetic(context);
    const bool keys = sampled && motion.has_keys(state.node, channel);
    if (component == 0) {
        object_draw::translate_current(context, keys ? sample_vector(motion, state, channel, fp)
                                                     : node.position);
    } else if (component == 1) {
        object_draw::rotate_current(context, state.rotation_order,
                                    keys ? sample_angles(motion, state, channel, fp)
                                         : node.rotation);
    } else {
        object_draw::scale_current(context,
                                   keys ? sample_vector(motion, state, channel, fp) : node.scale);
    }
    if (sampled)
        ++state.channel;
}
} // namespace

bool drawing_enabled(object_draw::State rendering) { return rendering.pending_logic_ticks == 0; }
void setup_motion(Motion *motion, float frame, State &state, fidelity::Context &fp) {
    if (motion) {
        const auto period = motion->frame_count();
        if (period == 0)
            throw std::runtime_error("animated_model_zero_motion_frames");
        const auto integer = static_cast<std::uint32_t>(ExactFloat(fp, frame).truncated_integer());
        const auto elapsed = (integer / period) * period;
        state.motion_type = motion->type();
        frame = (ExactFloat(fp, frame) - unsigned_value(fp, elapsed)).rounded();
        state.interpolation = motion->interpolation() & 65343U;
    } else {
        state.interpolation = 0;
        state.motion_type = 0;
    }
    state.node = 0;
    state.channel = 0;
    state.frame = frame;
    state.motion_auxiliary_index = 0;
}
void select_transforms(ObjectId root, State &state, object_draw::Context &context) {
    if (state.interpolation != 2 && state.interpolation != 3)
        throw std::runtime_error("animated_model_unrecovered_layout");
    state.rotation_order = (object_draw::object(context, root).evaluation_flags & 32U) != 0
                               ? object_draw::RotationOrder::Alternate
                               : object_draw::RotationOrder::Default;
}
void transform_mdata2(ObjectId id, Motion &motion, State &state, object_draw::Context &context) {
    state.channel = 0;
    transform_channel(id, 0, motion, state, context);
    transform_channel(id, 1, motion, state, context);
    transform_channel(id, 2, motion, state, context);
    ++state.node;
}
void transform_mdata3(ObjectId id, Motion &motion, State &state, object_draw::Context &context) {
    // NOTSA: identical channel operations; Motion resolves the three-pointer/count layout.
    transform_mdata2(id, motion, state, context);
}
void process_nodes(ObjectId root, Motion &motion, State &state, object_draw::State rendering,
                   object_draw::Context &context) {
    do {
        object_draw::push_matrix_ex(context);
        const auto &node = object_draw::object(context, root);
        if ((node.evaluation_flags & 64U) == 0) {
            if (state.interpolation == 3)
                transform_mdata3(root, motion, state, context);
            else
                transform_mdata2(root, motion, state, context);
        } else {
            object_draw::translate_current(context, node.position);
            object_draw::rotate_current(context,
                                        (node.evaluation_flags & 32U) != 0
                                            ? object_draw::RotationOrder::Alternate
                                            : object_draw::RotationOrder::Default,
                                        node.rotation);
            object_draw::scale_current(context, node.scale);
        }
        if (state.object_callback != ObjectCallback::None)
            object_callback(context, root);
        if (node.model != 0 && (node.evaluation_flags & 8U) == 0) {
            if (state.clip_scale == 0 || check_view(context, node.model, state.clip_scale)) {
                if (state.model_callback != ModelCallback::DrawModel)
                    throw std::runtime_error("animated_model_missing_model_callback");
                object_draw::draw_callback_model(node.model, rendering, context);
            }
        }
        if (node.child != 0)
            process_nodes(node.child, motion, state, rendering, context);
        object_draw::pop_matrix_ex(context);
        root = node.sibling;
    } while (root != 0);
}
void draw_action(ObjectId root, Motion *motion, float frame, std::uint32_t flags, State &state,
                 object_draw::State rendering, object_draw::Context &context) {
    if (!drawing_enabled(rendering) || root == 0 || motion == nullptr)
        return;
    if (object_draw::texture_list_not_ready(context))
        return;
    state.model_callback = ModelCallback::DrawModel;
    state.flags = flags;
    state.clip_scale = 0;
    setup_motion(motion, frame, state, arithmetic(context));
    select_transforms(root, state, context);
    process_nodes(root, *motion, state, rendering, context);
    state.clip_scale = 0;
}
void action_clip_no_scale(ObjectId root, Motion *motion, float frame, State &state,
                          object_draw::State rendering, object_draw::Context &context) {
    draw_action(root, motion, frame, 0, state, rendering, context);
}
} // namespace sadx::animated_model
