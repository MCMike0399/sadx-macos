#include "game/main_menu_content.hpp"
#include <algorithm>
#include <cstring>
namespace c = sadx::main_menu_content;
namespace {
float number(std::uint32_t a) { float v; std::memcpy(&v, &a, 4); return v; }
std::uint32_t bits(float a) { std::uint32_t v; std::memcpy(&v, &a, 4); return v; }
struct Calls final : c::Calls {
    std::uint32_t *out;
    bool optional;
    unsigned count = 0;
    Calls(std::uint32_t *o, bool available) : out(o), optional(available) {}
    void receipt(std::initializer_list<std::uint32_t> values) {
        std::copy(values.begin(), values.end(), out + 8 + count * 8);
        out[0] = ++count;
    }
    void select_common_texture() override { receipt({1, 19}); }
    void set_material(std::uint32_t a) override { receipt({2, a}); }
    bool optional_entry_available() override { receipt({3}); return optional; }
    void draw_scaled(std::uint32_t id, float x, float y, float z, float sx, float sy) override {
        receipt({4, id, bits(x), bits(y), bits(z), bits(sx), bits(sy)});
    }
};
}
extern "C" void sadx_native_main_menu_content_probe(const std::uint32_t *i, std::uint32_t *o) {
    std::fill_n(o, 64, 0);
    sadx::menu_dialog::ContentArguments args{number(i[3]), number(i[4]), number(i[5]), number(i[6]), number(i[7]), 0};
    const float sx = number(i[8]), sy = number(i[9]);
    const c::Layout layout{number(i[10]), number(i[11]), number(i[12]), number(i[13])};
    Calls calls(o, i[1] != 0);
    sadx::X87Context fp{static_cast<std::uint16_t>(i[2])};
    if (i[0]) c::draw_trial(args, {sx, sy, layout}, calls, fp);
    else c::draw_main(args, {sx, sy, layout}, calls, fp);
    o[1] = fp.exception_flags();
}
