#include "ninja_queue_initial_state_probe.hpp"
#include <cstddef>
#include <cstring>
#include <type_traits>
namespace sadx::test {
std::array<std::uint32_t, 21> queue_initial_words(const NinjaQueueInitialState &values) {
    std::array<std::uint32_t, 21> words{};
    unsigned next = 0;
    const auto store = [&](auto value) {
        using T = decltype(value);
        if constexpr (std::is_same_v<T, float>)
            std::memcpy(&words[next], &value, sizeof(value));
        else
            words[next] = static_cast<std::uint32_t>(value);
        ++next;
    };
    store(values.depth_comparison);
    store(values.execution_mode);
    store(values.lighting_palette);
    store(values.lighting_copy);
    store(values.packed_blend);
    store(values.primary_count);
    store(values.primary_high_water);
    store(values.secondary_count);
    store(values.secondary_high_water);
    for (auto value : values.reset_source)
        store(value);
    for (auto value : values.reset_capture)
        store(value);
    for (auto value : values.allocation_source)
        store(value);
    for (auto value : values.allocation_capture)
        store(value);
    return words;
}
NinjaQueueInitialState varied_queue_initial_state() {
    auto values = ninja_queue_initial_state();
    std::uint32_t ordinal = 0, float_ordinal = 0;
    const auto vary = [&](auto &value) {
        const auto index = ++ordinal;
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, float>) {
            const std::uint32_t special[] = {0x80000000U, 0x7f800000U, 0xff800000U, 0x7fc12345U,
                                             0x7fa12345U, 1U,          0x80000001U};
            const auto floating = ++float_ordinal;
            const auto word = floating <= 7 ? special[floating - 1] : 0x3e800000U + 8111U * index;
            std::memcpy(&value, &word, sizeof(word));
        } else if constexpr (std::is_same_v<T, std::int32_t>) {
            value = -static_cast<std::int32_t>(1234567U * index);
        } else {
            value = static_cast<T>(1234567U * index);
        }
    };
    vary(values.depth_comparison);
    vary(values.execution_mode);
    vary(values.lighting_palette);
    vary(values.lighting_copy);
    vary(values.packed_blend);
    vary(values.primary_count);
    vary(values.primary_high_water);
    vary(values.secondary_count);
    vary(values.secondary_high_water);
    for (auto &value : values.reset_source)
        vary(value);
    for (auto &value : values.reset_capture)
        vary(value);
    for (auto &value : values.allocation_source)
        vary(value);
    for (auto &value : values.allocation_capture)
        vary(value);
    return values;
}
} // namespace sadx::test
extern "C" std::size_t sadx_native_ninja_queue_initial_state(unsigned variant,
                                                             std::uint32_t *output,
                                                             std::size_t capacity) {
    if (variant > 1 || !output || capacity < 21)
        return 0;
    const auto values =
        variant == 0 ? sadx::ninja_queue_initial_state() : sadx::test::varied_queue_initial_state();
    const auto words = sadx::test::queue_initial_words(values);
    for (std::size_t i = 0; i < words.size(); ++i)
        output[i] = words[i];
    return words.size();
}
