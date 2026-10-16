#include "ninja_screen_initial_state_probe.hpp"
#include <cstddef>
#include <cstring>
#include <type_traits>

namespace sadx::test {
std::array<std::uint32_t, 90> screen_initial_words(const NinjaScreenInitialState &values) {
    const auto bits = [](float value) {
        std::uint32_t word;
        std::memcpy(&word, &value, sizeof(word));
        return word;
    };
    std::array<std::uint32_t, 90> words{};
    unsigned next = 0;
    words[next++] = bits(values.clip.near_distance);
    words[next++] = bits(values.clip.far_distance);
    words[next++] = bits(values.clip.range_near);
    words[next++] = bits(values.clip.range_far);
    words[next++] = bits(values.clip.cached_near);
    words[next++] = bits(values.clip.cached_far);
    words[next++] = values.setup_argument;
    words[next++] = values.flags;
    words[next++] = values.timing_multiplier;
    words[next++] = values.width_pixels;
    words[next++] = values.height_pixels;
    words[next++] = bits(values.logical_height);
    words[next++] = bits(values.viewport_width);
    words[next++] = bits(values.viewport_height);
    words[next++] = bits(values.screen_bounds.left);
    words[next++] = bits(values.screen_bounds.top);
    words[next++] = bits(values.screen_bounds.right);
    words[next++] = bits(values.screen_bounds.bottom);
    words[next++] = values.transform_invalid;
    words[next++] = values.polygon_setup_word;
    words[next++] = values.polygon_count;
    words[next++] = values.viewport.x;
    words[next++] = values.viewport.y;
    words[next++] = values.viewport.width;
    words[next++] = values.viewport.height;
    words[next++] = bits(values.viewport.min_z);
    words[next++] = bits(values.viewport.max_z);
    words[next++] = bits(values.half_width);
    words[next++] = bits(values.half_height);
    words[next++] = bits(values.viewport_bounds.left);
    words[next++] = bits(values.viewport_bounds.top);
    words[next++] = bits(values.viewport_bounds.right);
    words[next++] = bits(values.viewport_bounds.bottom);
    words[next++] = bits(values.projection_width);
    words[next++] = bits(values.polygon_points[0].x);
    words[next++] = bits(values.polygon_points[0].y);
    words[next++] = bits(values.polygon_points[1].x);
    words[next++] = bits(values.polygon_points[1].y);
    words[next++] = bits(values.polygon_points[2].x);
    words[next++] = bits(values.polygon_points[2].y);
    words[next++] = bits(values.polygon_points[3].x);
    words[next++] = bits(values.polygon_points[3].y);
    for (auto value : values.projection)
        words[next++] = bits(value);
    for (auto value : values.work_transform)
        words[next++] = bits(value);
    for (auto value : values.base_transform)
        words[next++] = bits(value);
    return words;
}
NinjaScreenInitialState varied_screen_initial_state() {
    auto values = ninja_screen_initial_state();
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
    vary(values.clip.near_distance);
    vary(values.clip.far_distance);
    vary(values.clip.range_near);
    vary(values.clip.range_far);
    vary(values.clip.cached_near);
    vary(values.clip.cached_far);
    vary(values.setup_argument);
    vary(values.flags);
    vary(values.timing_multiplier);
    vary(values.width_pixels);
    vary(values.height_pixels);
    vary(values.logical_height);
    vary(values.viewport_width);
    vary(values.viewport_height);
    vary(values.screen_bounds.left);
    vary(values.screen_bounds.top);
    vary(values.screen_bounds.right);
    vary(values.screen_bounds.bottom);
    vary(values.transform_invalid);
    vary(values.polygon_setup_word);
    vary(values.polygon_count);
    vary(values.viewport.x);
    vary(values.viewport.y);
    vary(values.viewport.width);
    vary(values.viewport.height);
    vary(values.viewport.min_z);
    vary(values.viewport.max_z);
    vary(values.half_width);
    vary(values.half_height);
    vary(values.viewport_bounds.left);
    vary(values.viewport_bounds.top);
    vary(values.viewport_bounds.right);
    vary(values.viewport_bounds.bottom);
    vary(values.projection_width);
    vary(values.polygon_points[0].x);
    vary(values.polygon_points[0].y);
    vary(values.polygon_points[1].x);
    vary(values.polygon_points[1].y);
    vary(values.polygon_points[2].x);
    vary(values.polygon_points[2].y);
    vary(values.polygon_points[3].x);
    vary(values.polygon_points[3].y);
    for (auto &value : values.projection)
        vary(value);
    for (auto &value : values.work_transform)
        vary(value);
    for (auto &value : values.base_transform)
        vary(value);
    return values;
}
} // namespace sadx::test

extern "C" std::size_t sadx_native_ninja_screen_initial_state(unsigned variant,
                                                              std::uint32_t *output,
                                                              std::size_t capacity) {
    if (variant > 1 || !output || capacity < 90)
        return 0;
    const auto values = variant == 0 ? sadx::ninja_screen_initial_state()
                                     : sadx::test::varied_screen_initial_state();
    const auto words = sadx::test::screen_initial_words(values);
    for (std::size_t i = 0; i < words.size(); ++i)
        output[i] = words[i];
    return words.size();
}
