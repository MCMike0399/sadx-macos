#include "ninja_sine_table_probe.hpp"
#include <cstddef>
#include <cstring>

namespace sadx::test {
std::array<std::uint32_t, 1025> sine_words(const NinjaQuarterSineTable &table) {
    std::array<std::uint32_t, 1025> words{};
    for (std::size_t i = 0; i < table.size(); ++i)
        std::memcpy(&words[i], &table[i], sizeof(words[i]));
    return words;
}
NinjaQuarterSineTable varied_sine_table() {
    NinjaQuarterSineTable table{};
    for (std::uint32_t i = 0; i < table.size(); ++i) {
        const auto bits = 0x3e800000U + 8111U * i;
        std::memcpy(&table[i], &bits, sizeof(bits));
    }
    // Constructor transport only: include zeros, infinities and both NaN classes.
    const std::uint32_t special[] = {0,           0x80000000U, 0x7f800000U, 0xff800000U,
                                     0x7fc12345U, 0x7fa12345U, 1,           0x80000001U};
    for (std::size_t i = 0; i < 8; ++i)
        std::memcpy(&table[i], &special[i], sizeof(special[i]));
    return table;
}
} // namespace sadx::test
extern "C" std::size_t sadx_native_ninja_sine_table(unsigned variant, std::uint32_t *output,
                                                    std::size_t capacity) {
    if (variant > 1 || !output || capacity < 1025)
        return 0;
    const auto table = variant == 0 ? sadx::ninja_sine_table() : sadx::test::varied_sine_table();
    const auto words = sadx::test::sine_words(table);
    for (std::size_t i = 0; i < words.size(); ++i)
        output[i] = words[i];
    return words.size();
}
