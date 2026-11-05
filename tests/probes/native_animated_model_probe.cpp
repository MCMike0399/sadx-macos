#include "engine/animated_model.hpp"
#include <algorithm>
#include <cstring>
#include <map>
#include <vector>

namespace {
constexpr std::uint32_t kObjectBase = 0x11000400;
constexpr std::uint32_t kModelBase = 0x11000600;
constexpr std::uint32_t kDrawCallback = 0x403470;
// NOTSA: reference transport preserves words without arithmetic conversion.
template <class T> T value(std::uint32_t word) {
    T result;
    std::memcpy(&result, &word, 4);
    return result;
}
std::uint32_t word(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
struct Motion final : sadx::animated_model::Motion {
    const std::uint32_t *input;
    explicit Motion(const std::uint32_t *input) : input(input) {}
    std::uint32_t frame_count() override { return 12; }
    std::uint16_t type() override { return input[5]; }
    std::uint16_t interpolation() override { return input[6]; }
    std::uint32_t key_count(unsigned node, unsigned channel) override {
        return input[68 + (node * 2 + channel) * 17];
    }
    std::uint32_t key_word(unsigned node, unsigned channel, unsigned key, unsigned field) override {
        return input[69 + (node * 2 + channel) * 17 + key * 4 + field];
    }
};
} // namespace
namespace sadx::object_draw {
struct MatrixBoundary {};
struct Context {
    const std::uint32_t *input;
    animated_model::State &animated;
    std::map<ObjectId, Object> objects;
    std::vector<std::uint32_t> trace;
    fidelity::Context fp{127};
    Context(const std::uint32_t *input, animated_model::State &animated)
        : input(input), animated(animated) {}
    void event(std::uint32_t tag, std::initializer_list<std::uint32_t> words = {}) {
        trace.push_back(tag);
        trace.insert(trace.end(), words);
        trace.insert(trace.end(), 5 - words.size(), 0);
        if (input[14] != 0 && tag == input[14])
            throw MatrixBoundary{};
    }
};
const Object &object(Context &context, ObjectId id) { return context.objects.at(id); }
void push_matrix_ex(Context &context) { context.event(2); }
void translate_current(Context &context, const std::array<float, 3> &position) {
    context.event(3, {word(position[0]), word(position[1]), word(position[2])});
}
void rotate_current(Context &context, RotationOrder order, std::array<std::int32_t, 3> rotation) {
    context.event(4, {std::uint32_t(order == RotationOrder::Alternate),
                      std::uint32_t(rotation[0]), std::uint32_t(rotation[1]),
                      std::uint32_t(rotation[2])});
}
void scale_current(Context &context, const std::array<float, 3> &scale) {
    context.event(5, {word(scale[0]), word(scale[1]), word(scale[2])});
}
void pop_matrix_ex(Context &context) { context.event(8); }
bool texture_list_not_ready(Context &context) {
    context.event(1);
    return context.input[1] != 0;
}
void draw_late_object(Context &, ObjectId, std::uint32_t) { throw 1; }
void draw_model(Context &context, ModelId id) {
    const auto &state = context.animated;
    context.event(7, {id, state.flags, word(state.clip_scale),
                      state.model_callback == animated_model::ModelCallback::DrawModel
                          ? kDrawCallback : 0});
}
} // namespace sadx::object_draw
namespace sadx::animated_model {
fidelity::Context &arithmetic(object_draw::Context &context) { return context.fp; }
void object_callback(object_draw::Context &context, ObjectId id) {
    context.event(6, {id});
    auto &object = context.objects.at(id);
    if (context.input[8] == 1)
        object.model = kModelBase + 3 * 64;
    else if (context.input[8] == 2)
        object.evaluation_flags ^= 8;
    else if (context.input[8] == 3 && id == kObjectBase + 3 * 64)
        context.animated.clip_scale = 0.75f;
}
bool check_view(object_draw::Context &, object_draw::ModelId, float) { return true; }
} // namespace sadx::animated_model

extern "C" std::uint32_t animated_model_probe(const std::uint32_t *input,
                                               std::uint32_t *output) {
    using namespace sadx;
    animated_model::State state;
    state.flags = 91;
    state.clip_scale = value<float>(input[9]);
    state.frame = -17;
    state.channel = 79;
    state.motion_auxiliary_index = 83;
    state.object_callback = input[7] ? animated_model::ObjectCallback::Gamma
                                    : animated_model::ObjectCallback::None;
    object_draw::Context context(input, state);
    for (unsigned i = 0; i < 4; ++i) {
        const auto *row = input + 16 + i * 13;
        context.objects.emplace(kObjectBase + i * 64,
            object_draw::Object{row[0], row[1],
                               {value<float>(row[2]), value<float>(row[3]), value<float>(row[4])},
                               {value<std::int32_t>(row[5]), value<std::int32_t>(row[6]),
                                value<std::int32_t>(row[7])},
                               {value<float>(row[8]), value<float>(row[9]), value<float>(row[10])},
                               row[11], row[12]});
    }
    Motion motion(input);
    std::uint8_t fog = 0, fog_active = 0, simple = input[12];
    float depth = 0;
    const std::int32_t ticks = input[0], palette = input[13];
    object_draw::State rendering{fog, fog_active, depth, ticks, simple, palette};
    try {
        const auto root = input[2] ? kObjectBase : 0;
        auto *source = input[3] ? &motion : nullptr;
        if (input[11])
            animated_model::action_clip_no_scale(root, source, value<float>(input[4]),
                                                 state, rendering, context);
        else
            animated_model::draw_action(root, source, value<float>(input[4]), input[10],
                                         state, rendering, context);
    } catch (const object_draw::MatrixBoundary &) {
        // The original judge stops at the same common numeric leaf boundary.
    } catch (...) {
        return 0xffffffffU;
    }
    std::vector<std::uint32_t> result{
        state.model_callback == animated_model::ModelCallback::DrawModel ? kDrawCallback : 0,
        state.flags, word(state.clip_scale), word(state.frame), state.node, state.channel,
        state.motion_type, state.interpolation, state.motion_auxiliary_index};
    for (unsigned i = 0; i < 4; ++i) {
        const auto &object = context.objects.at(kObjectBase + i * 64);
        result.push_back(object.evaluation_flags);
        result.push_back(object.model);
        for (float f : object.position) result.push_back(word(f));
        for (auto a : object.rotation) result.push_back(std::uint32_t(a));
        for (float f : object.scale) result.push_back(word(f));
        result.push_back(object.child);
        result.push_back(object.sibling);
    }
    result.push_back(context.trace.size());
    result.insert(result.end(), context.trace.begin(), context.trace.end());
    if (result.size() > 2048) return 0xfffffffeU;
    std::copy(result.begin(), result.end(), output);
    return result.size();
}
