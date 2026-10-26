#include "../probes/basic_model_backend.hpp"
#include <cassert>
#include <iostream>

namespace bm = sadx::basic_model;
int main() {
    std::array<std::uint32_t, 144> input{};
    input[18] = 127;
    bm::State state;
    std::uint32_t control = 0, mask = ~std::uint32_t{0}, attributes = 0;
    std::array<float, 4> constant{1, 0.2f, 0.4f, 0.6f};
    sadx::graphics::Matrix current{};
    sadx::D3dxMatrix inverse{}, reset{};
    std::uint32_t current_word = 123;
    std::memcpy(current.data(), &current_word, sizeof(current_word));
    inverse[0] = 456;
    reset[0] = 789;
    bm::Owners owners{state, control, mask, attributes, constant, current, inverse, reset};
    bm::Context context(state, input.data());
    bm::parse_material({0xa1223344U, 0xb1556677U, 12, 0x10005, 1U << 21}, owners, context);
    assert(state.landtable_color == 0xa1223344U && state.vertex_color == 0xa1223344U);
    const std::vector<std::uint32_t> expected{3,
                                              17,
                                              bm::probe_bits(34),
                                              bm::probe_bits(51),
                                              bm::probe_bits(68),
                                              bm::probe_bits(161),
                                              bm::probe_bits(34),
                                              bm::probe_bits(51),
                                              bm::probe_bits(68),
                                              bm::probe_bits(161),
                                              bm::probe_bits(85),
                                              bm::probe_bits(102),
                                              bm::probe_bits(119),
                                              bm::probe_bits(177),
                                              0,
                                              0,
                                              0,
                                              0,
                                              bm::probe_bits(12),
                                              2,
                                              2,
                                              13,
                                              1,
                                              2,
                                              2,
                                              14,
                                              1,
                                              1,
                                              2,
                                              19,
                                              1,
                                              1,
                                              2,
                                              20,
                                              1,
                                              8,
                                              1,
                                              5,
                                              1,
                                              2,
                                              137,
                                              1,
                                              1,
                                              2,
                                              27,
                                              0,
                                              2,
                                              2,
                                              4,
                                              3,
                                              1,
                                              2,
                                              24,
                                              0};
    assert(context.events == expected);
    context.events.clear();
    state.render_flags = 23;
    input[20] = 16; // reset callback changes a high flag before the mask is reread
    bm::reset_render_flags(state, context);
    assert(state.render_flags == 0);
    assert(context.events[0] == 2 && context.events[2] == 24 && context.events[3] == 0);
    assert(context.events[4] == 21 && context.events[6] == bm::probe_bits(1));
    assert(context.events[22] == 2 && context.events[24] == 11 && context.events[25] == 0);
    context.events.clear();
    control = 16;
    bm::copy_constant_material(owners, context);
    assert(state.cull_mode == 1 && state.constant_color == 0xff336699U);
    assert((state.render_flags & 2) != 0);
    const auto saved = state.inverse_current;
    input[23] = 1;
    current_word = 999;
    std::memcpy(current.data(), &current_word, sizeof(current_word));
    bm::prepare_matrices(owners, context);
    assert(state.inverse_current == saved);
    assert(state.current_no_view[0] == (999U ^ 456U));
    context.events.clear();
    bm::reset_world_transform(owners, context);
    assert(context.events[2] == 789);
    bool stopped = false;
    try {
        throw bm::Unavailable(bm::Boundary::Buffer);
    } catch (const bm::Unavailable &failure) {
        stopped = failure.boundary == bm::Boundary::Buffer;
    }
    assert(stopped);
    std::cout << "Material byte channels, source call order, live reset mask, constant packing, "
                 "singular preservation, distinct reset source and tagged stop pass.\n";
}
