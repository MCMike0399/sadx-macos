#include "player/gamma_display.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace {
template <class T> T value(std::uint32_t word) {
    T result;
    std::memcpy(&result, &word, 4);
    return result;
}
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
} // namespace
namespace sadx::gamma_display {
struct Context {
    const std::uint32_t *input;
    fidelity::Context fp{127};
    std::vector<std::uint32_t> trace;
    bool ordinary = false;
    explicit Context(const std::uint32_t *input, bool ordinary = false)
        : input(input), ordinary(ordinary) {}
    void event(std::uint32_t tag, std::uint32_t a = 0, std::uint32_t b = 0, std::uint32_t c = 0) {
        trace.insert(trace.end(), {tag, a, b, c, 0});
    }
};
ObjectTable object_table(Context &context) { return &context; }
ObjectId object_row(Context &context, ObjectTable, unsigned index) {
    switch (index) {
    case 11:
        return context.input[1];
    case 12:
        return context.input[2];
    case 28:
        return context.input[3];
    default:
        throw 1;
    }
}
std::int32_t aim_rotation(Context &context) { return value<std::int32_t>(context.input[4]); }
float aim_translation(Context &context) { return value<float>(context.input[5]); }
fidelity::ExactFloat exact(Context &context, float value) { return {context.fp, value}; }
void rotate(Context &context, Axis axis, std::uint16_t angle) {
    if (axis != Axis::Y)
        throw 1;
    context.trace.insert(context.trace.end(), {1, angle, 0, 0, 0});
}
void translate_numeric(Context &context, const NinjaVector &translation) {
    context.trace.insert(context.trace.end(),
                         {2, bits(translation.x), bits(translation.y), bits(translation.z), 0});
}
float animation_frame(Context &context) {
    context.event(10, context.input[2]);
    return value<float>(context.input[2]);
}
std::int16_t animation_mode(Context &context) {
    context.event(11, context.input[1]);
    return static_cast<std::int16_t>(context.input[1]);
}
void draw_generated_action(Context &context, float frame) {
    context.event(13, context.input[4], bits(frame));
}
void draw_table_action(Context &context, std::uint16_t animation, float frame) {
    context.event(12, context.input[3] + animation * 64, bits(frame));
}
void clear_object_callback(Context &context) { context.event(14); }
void push_current(Context &context) { context.event(15); }
void multiply_arm_matrix(Context &context) { context.event(16, 1); }
std::int32_t arm_rotation(Context &context) {
    context.event(17, context.input[6]);
    return value<std::int32_t>(context.input[6]);
}
struct ArmStop {};
void complete_arm(Context &c) { draw_arm_action(c); }
void calculate_arm_matrix(Context &) { throw ArmStop{}; }
void pop(Context &) { throw ArmStop{}; }
void draw_arm_action(Context &context) {
    context.event(18);
    throw ArmStop{};
}
} // namespace sadx::gamma_display

extern "C" std::uint32_t gamma_object_callback_probe(const std::uint32_t *input,
                                                     std::uint32_t *output) {
    sadx::gamma_display::Context context(input);
    sadx::gamma_display::gamma_object_callback(context, input[0]);
    output[0] = context.fp.exception_flags();
    output[1] = static_cast<std::uint32_t>(context.trace.size());
    std::copy(context.trace.begin(), context.trace.end(), output + 2);
    return static_cast<std::uint32_t>(context.trace.size() + 2);
}

extern "C" std::uint32_t gamma_ordinary_body_probe(const std::uint32_t *input,
                                                   std::uint32_t *output) {
    sadx::gamma_display::Context context(input, true);
    try {
        sadx::gamma_display::ordinary_body(context, static_cast<std::uint16_t>(input[0]));
    } catch (const sadx::gamma_display::ArmStop &) {
    }
    output[0] = context.fp.exception_flags();
    output[1] = static_cast<std::uint32_t>(context.trace.size());
    std::copy(context.trace.begin(), context.trace.end(), output + 2);
    return static_cast<std::uint32_t>(context.trace.size() + 2);
}
