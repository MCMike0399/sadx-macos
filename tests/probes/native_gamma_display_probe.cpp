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
    std::uint32_t primary = 0, extra = 0, collision = 0;
    bool body = false;
    fidelity::Context fp{0x007f};
    std::uint32_t peek(std::uint32_t address) { return output[index(address)]; }
    void trace(std::uint32_t op, std::uint32_t a = 0, std::uint32_t b = 0) {
        auto &count = output[1040];
        const auto at = 1041 + count++ * 3;
        if (count > 48) throw Stop{};
        output[at] = op;
        output[at + 1] = a;
        output[at + 2] = b;
    }
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
void display_tail(Context &, const NinjaVector &) { throw Stop{}; }
void capture(Context &context, const TaskHandle &) {
    context.primary = context.get(context.input[0] + 32);
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
void capture_extra(Context &context) { context.extra = context.get(context.player + 124); }
ObjectId sibling(Context &context, ObjectId object) { return context.get(object + 48); }
ObjectId child(Context &context, ObjectId object) { return context.get(object + 44); }
void store_sibling(Context &context, ObjectId object, ObjectId value) {
    context.store(object + 48, value);
}
void store_weld_source(Context &context, ObjectId value) { context.store(kWeld, value); }
namespace {
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
float real(std::uint32_t value) {
    float result;
    std::memcpy(&result, &value, 4);
    return result;
}
}
void select_palette(Context &context, std::int32_t value) {
    if (!context.input[5]) throw Stop{};
    context.body = true;
    context.trace(1, value);
}
std::uint16_t requested_animation(Context &c) { return c.peek(c.player + 292); }
std::uint16_t current_animation(Context &c) { return c.peek(c.player + 292) >> 16; }
std::int16_t animation_mode(Context &c) { return c.peek(c.player + 288); }
std::uint16_t status(Context &c) { return c.peek(c.primary + 4) >> 16; }
float motion_height(Context &c) { return real(c.peek(c.player + 284)); }
std::int32_t angle(Context &c, Axis field) {
    return static_cast<std::int32_t>(c.peek(c.primary + 20 + static_cast<unsigned>(field) * 4));
}
float position(Context &c, Axis field) {
    return real(c.peek(c.primary + 32 + static_cast<unsigned>(field) * 4));
}
fidelity::ExactFloat exact(Context &c, float value) { return {c.fp, value}; }
void select_body_textures(Context &c) { c.trace(2, 0x91c560); }
void save_control(Context &c) { c.output[1029] = c.output[1026]; c.trace(3); }
void set_control(Context &c, std::uint32_t value) { c.output[1026] = value; c.trace(4, value); }
void set_white_material(Context &c) {
    for (unsigned i = 1032; i < 1036; ++i) c.output[i] = bits(1.0f);
    c.trace(5, bits(1.0f), bits(1.0f));
}
void save_attributes(Context &c) {
    c.output[1030] = c.output[1027]; c.output[1031] = c.output[1028]; c.trace(6);
}
void clear_attributes(Context &c, std::uint32_t mask, std::uint32_t attributes) {
    c.output[1027] &= ~mask; c.output[1028] &= ~attributes; c.trace(7, mask, attributes);
}
void push_current(Context &c) { c.trace(8); }
void push_unit(Context &c) { c.trace(9); }
void rotate(Context &c, Axis field, std::uint16_t value) {
    c.trace(10 + static_cast<unsigned>(field), value);
}
NinjaVector transform_vector(Context &c, const NinjaVector &value) {
    c.trace(13, bits(value.x), bits(value.y));
    c.trace(14, bits(value.z));
    // Explicit numeric-leaf seam: the original and native receive the same
    // transformed vector. The display judge verifies placement and ordering,
    // not the separately recovered Ninja matrix arithmetic.
    return {real(c.input[6]), value.y, real(c.input[7])};
}
void pop(Context &c) { c.trace(15); }
void translate(Context &c, const NinjaVector &value) {
    c.trace(16, bits(value.x), bits(value.y)); c.trace(17, bits(value.z));
}
void capture_collision_info(Context &c) { c.collision = c.peek(c.peek(c.primary + 56) + 12); }
void store_collision_center(Context &c, unsigned record, Axis field, float value) {
    c.put(c.collision + record * 48 + 8 + static_cast<unsigned>(field) * 4, bits(value));
}
void store_walk_phase(Context &c, std::uint32_t value) { c.put(c.extra + 44, value); }
bool has_event_action(Context &c) { return c.peek(c.peek(c.primary + 60) + 64) != 0; }
void set_object_callback(Context &c) { c.output[1036] = 0x47f990; c.trace(18, 0x47f990); }
[[noreturn]] void walking_sway(Context &c) { c.trace(19); throw Stop{}; }
[[noreturn]] void event_model(Context &c) { c.trace(20); throw Stop{}; }
[[noreturn]] void hidden_tail(Context &c) { c.trace(21); throw Stop{}; }
void draw_player_action(Context &c, std::uint16_t animation) {
    c.trace(22, animation); throw Stop{};
}

} // namespace sadx::gamma_display
extern "C" int sadx_native_gamma_display(const std::uint32_t *input, std::uint32_t *output) {
    std::copy_n(input + 8, 1024, output);
    output[1024] = output[1025] = 0;
    sadx::gamma_display::Context context{input, output};
    if (input[5]) {
        std::fill_n(output + 1026, 159, 0);
        for (unsigned i = 0; i < 10; ++i) output[1026 + i] = input[10 + i];
    }
    try { sadx::gamma_display::display(context, {}); }
    catch (const sadx::gamma_display::Stop &) {}
    return 1;
}
