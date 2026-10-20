#include "game/logo.hpp"
#include <algorithm>
#include <cstring>

namespace {
float number(std::uint32_t value) {
    float result;
    std::memcpy(&result, &value, 4);
    return result;
}
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
struct Stop {};
struct Calls final : sadx::logo::DisplayCalls {
    std::uint32_t *out;
    std::uint32_t stop, count = 0;
    Calls(std::uint32_t *out, std::uint32_t stop) : out(out), stop(stop) {}
    void record(std::uint32_t kind, std::initializer_list<std::uint32_t> values = {}) {
        out[4 + count * 8] = kind;
        std::copy(values.begin(), values.end(), out + 5 + count * 8);
        if (++count == stop)
            throw Stop{};
    }
    void select_logo_textures() override { record(1); }
    void set_material(std::uint32_t value) override { record(2, {value}); }
    void default_blending() override { record(3); }
    void set_base_color(std::uint32_t value) override { record(4, {value}); }
    void draw_texture(std::uint32_t texture, float x, float y, float depth) override {
        record(5, {texture, bits(x), bits(y), bits(depth)});
    }
    void draw_box(float width, float height, float depth, std::uint32_t color) override {
        record(6, {0, 0, bits(width), bits(height), bits(depth), color});
    }
};
} // namespace
extern "C" void sadx_native_logo_display(const std::uint32_t *input, std::uint32_t *output) {
    std::fill_n(output, 96, 0);
    auto frames = static_cast<std::int32_t>(input[0]);
    auto horizontal = number(input[1]), vertical = number(input[2]);
    auto color = input[3];
    sadx::logo::DisplayState state{frames, horizontal, vertical, color};
    sadx::X87Context fp(static_cast<std::uint16_t>(input[5]));
    Calls calls(output, input[4]);
    try {
        sadx::logo::display(static_cast<std::uint8_t>(input[6]), state, calls, fp);
        output[0] = 1;
    } catch (Stop) {
    } catch (const sadx::X87Error &) {
        output[0] = 2;
    }
    output[1] = calls.count;
    output[2] = fp.control_word();
    output[3] = fp.exception_flags();
}
