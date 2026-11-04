#include "player/gamma_display.hpp"
#include <algorithm>
#include <cstring>

namespace sadx::gamma_display {
namespace {
constexpr std::uint32_t kBase = 0x11000000, kTable = 0x3a8af6c, kWeld = 0x3c5455c;
struct Stop {};
}
struct Context {
    const std::uint32_t *input;
    std::uint32_t *output;
    std::uint32_t player = 0, step = 0;
    void access() {
        ++step;
        output[1025] = step;
        if (input[2] == step) put(input[3], input[4]);
        if (input[1] == step) fail();
    }
    [[noreturn]] void fail() {
        output[1024] = step;
        throw Stop{};
    }
    unsigned index(std::uint32_t address) {
        if (address == kTable) return 0;
        if (address == kWeld) return 1;
        if (address < kBase || address >= kBase + 4096 || (address & 3)) fail();
        return (address - kBase) / 4;
    }
    std::uint32_t get(std::uint32_t address) {
        access();
        return output[index(address)];
    }
    void put(std::uint32_t address, std::uint32_t value) { output[index(address)] = value; }
    void store(std::uint32_t address, std::uint32_t value) {
        access();
        put(address, value);
    }
};
void capture(Context &context, const TaskHandle &) {
    context.get(context.input[0] + 32);
    const auto motion = context.get(context.input[0] + 36);
    context.player = context.get(motion);
}
ObjectTable object_table(Context &context) {
    return reinterpret_cast<ObjectTable>(static_cast<std::uintptr_t>(context.get(kTable)));
}
ObjectId object_row(Context &context, ObjectTable table, unsigned row) {
    return context.get(static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(table)) + row * 4);
}
std::uint16_t equipment(Context &context) { return context.get(context.player + 4); }
void capture_extra(Context &context) { context.get(context.player + 124); }
ObjectId sibling(Context &context, ObjectId object) { return context.get(object + 48); }
ObjectId child(Context &context, ObjectId object) { return context.get(object + 44); }
void store_sibling(Context &context, ObjectId object, ObjectId value) {
    context.store(object + 48, value);
}
void store_weld_source(Context &context, ObjectId value) { context.store(kWeld, value); }
void select_palette(Context &, std::int32_t) { throw Stop{}; }
// The prefix judge stops at the first palette call in both programs. No later
// source behavior is simulated or claimed by this probe.
#define UNREACHED(name, result, parameters) result name parameters { throw Stop{}; }
UNREACHED(requested_animation, std::uint16_t, (Context &))
UNREACHED(current_animation, std::uint16_t, (Context &))
UNREACHED(animation_mode, std::int16_t, (Context &))
UNREACHED(status, std::uint16_t, (Context &))
UNREACHED(motion_height, float, (Context &))
UNREACHED(angle, std::int32_t, (Context &, Axis))
UNREACHED(position, float, (Context &, Axis))
UNREACHED(exact, fidelity::ExactFloat, (Context &, float))
UNREACHED(select_body_textures, void, (Context &))
UNREACHED(save_control, void, (Context &))
UNREACHED(set_control, void, (Context &, std::uint32_t))
UNREACHED(set_white_material, void, (Context &))
UNREACHED(save_attributes, void, (Context &))
UNREACHED(clear_attributes, void, (Context &, std::uint32_t, std::uint32_t))
UNREACHED(push_current, void, (Context &))
UNREACHED(push_unit, void, (Context &))
UNREACHED(rotate, void, (Context &, Axis, std::uint16_t))
UNREACHED(transform_vector, NinjaVector, (Context &, const NinjaVector &))
UNREACHED(pop, void, (Context &))
UNREACHED(translate, void, (Context &, const NinjaVector &))
UNREACHED(capture_collision_info, void, (Context &))
UNREACHED(store_collision_center, void, (Context &, unsigned, Axis, float))
UNREACHED(store_walk_phase, void, (Context &, std::uint32_t))
UNREACHED(has_event_action, bool, (Context &))
UNREACHED(set_object_callback, void, (Context &))
[[noreturn]] void walking_sway(Context &) { throw Stop{}; }
[[noreturn]] void event_model(Context &) { throw Stop{}; }
[[noreturn]] void hidden_tail(Context &) { throw Stop{}; }
[[noreturn]] void draw_player_action(Context &, std::uint16_t) { throw Stop{}; }
#undef UNREACHED
} // namespace sadx::gamma_display
extern "C" int sadx_native_gamma_display(const std::uint32_t *input, std::uint32_t *output) {
    std::copy_n(input + 8, 1024, output);
    output[1024] = output[1025] = 0;
    sadx::gamma_display::Context context{input, output};
    try { sadx::gamma_display::display(context, {}); }
    catch (const sadx::gamma_display::Stop &) {}
    return 1;
}
