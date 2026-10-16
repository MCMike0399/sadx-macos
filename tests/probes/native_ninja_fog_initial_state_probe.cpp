#include "ninja_fog_initial_state_probe.hpp"
#include <cstddef>
#include <cstring>
#include <type_traits>

namespace sadx::test {
std::array<std::uint32_t, 398> fog_initial_words(const NinjaFogInitialState &values) {
    const auto bits = [](float value) {
        std::uint32_t word;
        std::memcpy(&word, &value, sizeof(word));
        return word;
    };
    std::array<std::uint32_t, 398> words{};
    unsigned next = 0;
    words[next++] = values.enabled;
    words[next++] = values.table_supported;
    words[next++] = values.mode;
    words[next++] = bits(values.cached_start);
    words[next++] = bits(values.cached_end);
    words[next++] = values.device.color;
    words[next++] = values.device.start_index;
    words[next++] = values.device.end_index;
    words[next++] = bits(values.device.start_value);
    words[next++] = bits(values.device.end_value);
    words[next++] = values.device.software_density_bits;
    words[next++] = bits(values.level.start_z);
    words[next++] = bits(values.level.end_z);
    words[next++] = values.level.color;
    for (auto value : values.table)
        words[next++] = bits(value);
    for (auto value : values.device.w_depth)
        words[next++] = bits(value);
    for (auto value : values.device.reciprocal)
        words[next++] = bits(value);
    return words;
}
NinjaFogInitialState varied_fog_initial_state() {
    auto values = ninja_fog_initial_state();
    std::uint32_t ordinal = 1, float_ordinal = 0;
    const auto vary = [&](auto &value) {
        const auto index = ordinal++;
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, float>) {
            const std::uint32_t special[] = {0x80000000U, 0x7f800000U, 0xff800000U, 0x7fc12345U,
                                             0x7fa12345U, 1U,          0x80000001U};
            const auto floating = ++float_ordinal;
            const auto word = floating <= 7 ? special[floating - 1] : 0x3e800000U + 8111U * index;
            std::memcpy(&value, &word, sizeof(word));
        } else {
            value = index * 1234567U;
        }
    };
    vary(values.enabled);
    vary(values.table_supported);
    vary(values.mode);
    vary(values.cached_start);
    vary(values.cached_end);
    vary(values.device.color);
    vary(values.device.start_index);
    vary(values.device.end_index);
    vary(values.device.start_value);
    vary(values.device.end_value);
    vary(values.device.software_density_bits);
    vary(values.level.start_z);
    vary(values.level.end_z);
    vary(values.level.color);
    for (auto &value : values.table)
        vary(value);
    for (auto &value : values.device.w_depth)
        vary(value);
    for (auto &value : values.device.reciprocal)
        vary(value);
    return values;
}
} // namespace sadx::test

extern "C" std::size_t sadx_native_ninja_fog_initial_state(unsigned variant, std::uint32_t *output,
                                                           std::size_t capacity) {
    if (variant > 1 || !output || capacity < 398)
        return 0;
    const auto values =
        variant == 0 ? sadx::ninja_fog_initial_state() : sadx::test::varied_fog_initial_state();
    const auto words = sadx::test::fog_initial_words(values);
    for (std::size_t i = 0; i < words.size(); ++i)
        output[i] = words[i];
    return words.size();
}
