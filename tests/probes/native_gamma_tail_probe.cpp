#include "player/gamma_display.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace {
float real(std::uint32_t word) { float value; std::memcpy(&value, &word, 4); return value; }
std::uint32_t bits(float value) { std::uint32_t word; std::memcpy(&word, &value, 4); return word; }
}
namespace sadx::gamma_display {
struct Context {
    const std::uint32_t *input;
    fidelity::Context fp{127};
    std::vector<std::uint32_t> trace;
    std::uint32_t rotation, timer;
    NinjaVector scratch;
    std::array<NinjaVector, 6> vectors{};
    float eye = 0;
    unsigned alpha = 0, stops = 0;
    explicit Context(const std::uint32_t *in) : input(in), rotation(in[2]), timer(in[20]),
        scratch{real(in[16]), real(in[17]), real(in[18])} {}
    void event(unsigned tag, std::uint32_t a=0, std::uint32_t b=0, std::uint32_t c=0,
               std::uint32_t d=0) { trace.insert(trace.end(), {tag,a,b,c,d}); }
};
fidelity::ExactFloat exact(Context &c, float x) { return {c.fp,x}; }
std::uint16_t status(Context &c) { return c.input[3]; }
std::int8_t gun_mode(Context &c) { return c.input[4]; }
std::int8_t laser_mode(Context &c) { return c.input[5]; }
std::int16_t laser_lock_timer(Context &c) { return c.input[6]; }
std::int32_t angle(Context &c, Axis axis) { return c.input[7+unsigned(axis)]; }
float position(Context &c, Axis axis) { return real(c.input[10+unsigned(axis)]); }
void rotate(Context &c, Axis axis, std::uint16_t angle) { c.event(3,unsigned(axis),angle); }
void translate(Context &c, const NinjaVector &v) { c.event(4,bits(v.x),bits(v.y),bits(v.z)); }
void push_current(Context &c) { c.event(1,0); }
void push_unit(Context &c) { c.event(1,1); }
void push_arm_matrix(Context &c) { c.event(1,2); }
void pop(Context &c) { c.event(2); }
void multiply_arm_matrix(Context &c) { c.event(5,0); }
void multiply_attachment(Context &c, Attachment a) { c.event(5,unsigned(a)+1); }
NinjaVector &display_vector(Context &c) { return c.scratch; }
NinjaVector calculate_point(Context &c, const NinjaVector &p) {
    c.event(6,bits(p.x),bits(p.y),bits(p.z));
    return {1.25f,-2.5f,3.75f};
}
NinjaVector transform_vector(Context &c, const NinjaVector &p) {
    c.event(7,bits(p.x),bits(p.y),bits(p.z));
    return {-4,5,-6};
}
void normalize(Context &c, NinjaVector &p) { c.event(8,bits(p.x),bits(p.y),bits(p.z)); p={0.25f,0.5f,0.75f}; }
void store_extra_vector(Context &c, ExtraVector f, const NinjaVector &v) { c.vectors[unsigned(f)]=v; }
void store_arm_rotation(Context &c, std::int32_t v) { c.rotation=v; }
std::int32_t arm_rotation(Context &c) { return c.rotation; }
bool paused(Context &c) { c.event(9); return c.input[19]==16 || c.input[19]==25; }
void clear_display_flags(Context &c) { c.event(10); }
void store_eye_height(Context &c, float v) { c.eye=v; }
std::uint32_t headlight_timer(Context &c) { return c.timer; }
void store_headlight_timer(Context &c, std::uint32_t v) { c.timer=v; }
fidelity::ExactFloat sine(Context &c, std::int32_t angle) { c.event(11,angle); return exact(c,real(c.input[21])); }
void store_headlight_alpha(Context &c, std::uint8_t v) { c.alpha=v; }
std::uint16_t requested_animation(Context &c) { return c.input[22]; }
float animation_frame(Context &c) { return real(c.input[23]); }
void restore_attributes(Context &c) { c.event(12); }
void reset_material(Context &c) { c.event(13); }
void restore_control(Context &c) { c.event(14); }
void select_palette(Context &c, std::int32_t v) { c.event(15,v); }
struct Stop {};
[[noreturn]] void attachment_effect(Context &c) { c.stops=1; throw Stop{}; }
[[noreturn]] void gun_action(Context &c) { c.stops=2; throw Stop{}; }
[[noreturn]] void laser_target(Context &c) { c.stops=3; throw Stop{}; }
[[noreturn]] void headlight_fade(Context &c) { c.stops=4; throw Stop{}; }
[[noreturn]] void paused_shadow(Context &c) { c.stops=5; throw Stop{}; }
// Arm prefix uses the real saved-frame value and a distinct matrix-producer receipt.
void draw_arm_action(Context &c) { c.event(16,c.input[0],c.input[1]); }
void calculate_arm_matrix(Context &c) { c.event(17,c.input[0],c.input[1],2); }
}
extern "C" unsigned gamma_tail_probe(const std::uint32_t *input, std::uint32_t *output) {
    using namespace sadx;
    gamma_display::Context c(input);
    try {
        gamma_display::complete_arm(c);
        gamma_display::display_tail(c,{real(input[13]),real(input[14]),real(input[15])});
    } catch (const gamma_display::Stop &) {}
    std::vector<std::uint32_t> result{c.stops,c.rotation,c.timer,bits(c.eye),c.alpha,
                                    bits(c.scratch.x),bits(c.scratch.y),bits(c.scratch.z)};
    for (auto &v : c.vectors) result.insert(result.end(),{bits(v.x),bits(v.y),bits(v.z)});
    result.push_back(c.trace.size());
    result.insert(result.end(),c.trace.begin(),c.trace.end());
    result.push_back(c.fp.exception_flags());
    std::copy(result.begin(),result.end(),output);
    return result.size();
}
