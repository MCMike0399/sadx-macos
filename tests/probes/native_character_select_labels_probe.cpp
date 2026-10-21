#include "game/character_select_labels.hpp"
#include <algorithm>
#include <cstring>
namespace c = sadx::character_select_labels;
namespace {
float number(std::uint32_t word) { float value; std::memcpy(&value, &word, 4); return value; }
std::uint32_t bits(float value) { std::uint32_t word; std::memcpy(&word, &value, 4); return word; }
struct Calls final : c::Calls {
    std::uint32_t *out;
    unsigned count = 0;
    explicit Calls(std::uint32_t *output) : out(output) {}
    void receipt(std::initializer_list<std::uint32_t> row) {
        std::copy(row.begin(), row.end(), out + 8 + count * 8); out[0] = ++count;
    }
    void select_texture(c::Texture texture) override {
        receipt({1, texture == c::Texture::Labels ? 14U : texture == c::Texture::Names ? 15U : 28U});
    }
    void material(std::uint32_t color) override { receipt({2, color}); }
    void draw_texture(std::uint32_t id, float x, float y, float z) override {
        receipt({3, id, bits(x), bits(y), bits(z)});
    }
    void draw_scaled(std::uint32_t id, float x, float y, float z, float sx, float sy) override {
        receipt({4, id, bits(x), bits(y), bits(z), bits(sx), bits(sy)});
    }
};
}
extern "C" void sadx_native_character_select_labels_probe(const std::uint32_t *input, std::uint32_t *out) {
    std::fill_n(out, 64, 0);
    const float horizontal = number(input[8]), vertical = number(input[9]);
    const auto variant = static_cast<std::uint8_t>(input[10]);
    const auto language = static_cast<std::int32_t>(input[11]);
    std::array<std::uint8_t, 7> labels{}, names{};
    for (std::size_t index = 0; index < 7; ++index) {
        labels[index] = static_cast<std::uint8_t>(input[13 + index]);
        names[index] = static_cast<std::uint8_t>(input[20 + index]);
    }
    c::State state{horizontal, vertical, language, variant, labels, names};
    sadx::X87Context fp{static_cast<std::uint16_t>(input[12])};
    Calls calls(out);
    try {
        if (input[0]) c::draw_name(static_cast<std::int32_t>(input[1]), input[2], number(input[3]),
                                  number(input[4]), number(input[5]), number(input[6]), number(input[7]),
                                  state, calls, fp);
        else c::draw_label(static_cast<std::int32_t>(input[1]), input[2], number(input[3]),
                           number(input[4]), number(input[5]), state, calls, fp);
    } catch (const c::Error &error) { out[3] = error.boundary() == c::Boundary::LabelIndex ? 1U : 2U; }
    out[1] = fp.exception_flags(); out[2] = fp.control_word();
}
