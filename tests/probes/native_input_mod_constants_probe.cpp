#include "input_mod_constants_probe.hpp"
#include <cstddef>
namespace sadx::test {
std::array<std::uint32_t, 7> input_constant_words(const InputModPadConstants &values) {
    return {values.axes.epsilon.bits,        values.axes.maximum.bits,
            values.axes.keyboard_scale.bits, values.axes.controller_scale.bits,
            values.axes.magnitude_cap.bits,  values.trigger_scale.bits,
            values.half_press_scale.bits};
}
InputModPadConstants varied_input_constants() {
    return {{{0x80000000U}, {0x7f800000U}, {0xff800000U}, {0x7fc12345U}, {0x7fa12345U}},
            {1U},
            {0x80000001U}};
}
} // namespace sadx::test
extern "C" std::size_t sadx_native_input_mod_constants(unsigned variant, std::uint32_t *output,
                                                       std::size_t capacity) {
    if (variant > 1 || !output || capacity < 7)
        return 0;
    const auto values =
        variant == 0 ? sadx::input_mod_261_constants() : sadx::test::varied_input_constants();
    const auto words = sadx::test::input_constant_words(values);
    for (std::size_t i = 0; i < words.size(); ++i)
        output[i] = words[i];
    return words.size();
}
