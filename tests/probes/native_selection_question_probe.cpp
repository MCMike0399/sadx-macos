#include "game/selection_question.hpp"
#include <algorithm>
#include <cstring>
namespace q = sadx::selection_question;
namespace {
float number(std::uint32_t word) { float value; std::memcpy(&value, &word, 4); return value; }
std::uint32_t bits(float value) { std::uint32_t word; std::memcpy(&word, &value, 4); return word; }
struct Calls final : q::Calls {
    std::uint32_t *out;
    unsigned count = 0;
    explicit Calls(std::uint32_t *output) : out(output) {}
    void receipt(std::initializer_list<std::uint32_t> row) {
        std::copy(row.begin(), row.end(), out + 8 + count * 8); out[0] = ++count;
    }
    void push_current_matrix() override { receipt({1}); }
    void translate_current(const sadx::NinjaVector &p) override { receipt({2, bits(p.x), bits(p.y), bits(p.z)}); }
    void rotate_z(std::uint16_t angle) override { receipt({3, angle}); }
    void rotate_x(std::uint16_t angle) override { receipt({4, angle}); }
    void rotate_y(std::uint16_t angle) override { receipt({5, angle}); }
    void enable_specular() override { receipt({6, 29, 1}); }
    void select_menu_texture(std::uint32_t id) override { receipt({7, id}); }
    void draw_object(std::uint32_t id) override { receipt({8, id}); }
    void pop_matrix(std::uint32_t count) override { receipt({9, count}); }
};
}
extern "C" void sadx_native_selection_question_probe(const std::uint32_t *input, std::uint32_t *out) {
    std::fill_n(out, 96, 0);
    std::array<std::int32_t, 3> rotation{static_cast<std::int32_t>(input[1]),
        static_cast<std::int32_t>(input[2]), static_cast<std::int32_t>(input[3])};
    sadx::NinjaVector position{number(input[4]), number(input[5]), number(input[6])};
    const auto ticks = input[0];
    const std::uint32_t object = 2942, texture = 2;
    Calls calls(out);
    q::execute({rotation, position}, ticks, {object, texture}, calls);
    for (unsigned index = 0; index < 3; ++index) out[1 + index] = static_cast<std::uint32_t>(rotation[index]);
    out[4] = bits(position.x); out[5] = bits(position.y); out[6] = bits(position.z);
}
