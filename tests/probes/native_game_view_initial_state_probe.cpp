#include "game_view_initial_state_probe.hpp"
#include <cstddef>
#include <cstring>
#include <type_traits>

namespace sadx::test {
std::array<std::uint32_t, 30> view_initial_words(const GameViewInitialState &values) {
    std::array<std::uint32_t, 30> words{};
    unsigned next = 0;
    const auto store = [&](auto value) {
        using T = decltype(value);
        if constexpr (std::is_same_v<T, float>)
            std::memcpy(&words[next], &value, sizeof(value));
        else
            words[next] = static_cast<std::uint32_t>(value);
        ++next;
    };
    store(values.stretch_x);
    store(values.stretch_y);
    store(values.animation_queue_state);
    store(values.screen.distance);
    store(values.screen.width);
    store(values.screen.height);
    store(values.screen.center_x);
    store(values.screen.center_y);
    store(values.reset_pending);
    store(values.projection_center_x);
    store(values.projection_center_y);
    store(values.perspective);
    store(values.perspective_copy);
    store(values.last_perspective);
    for (auto color : values.clear_colors)
        store(color);
    for (const auto &color : {values.view_color, values.reset_color, values.sprite_color}) {
        store(color.alpha);
        store(color.red);
        store(color.green);
        store(color.blue);
    }
    return words;
}
GameViewInitialState varied_view_initial_state() {
    auto values = game_view_initial_state();
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
    vary(values.stretch_x);
    vary(values.stretch_y);
    vary(values.animation_queue_state);
    vary(values.screen.distance);
    vary(values.screen.width);
    vary(values.screen.height);
    vary(values.screen.center_x);
    vary(values.screen.center_y);
    vary(values.reset_pending);
    vary(values.projection_center_x);
    vary(values.projection_center_y);
    vary(values.perspective);
    vary(values.perspective_copy);
    vary(values.last_perspective);
    for (auto &color : values.clear_colors)
        vary(color);
    for (auto *color : {&values.view_color, &values.reset_color, &values.sprite_color}) {
        vary(color->alpha);
        vary(color->red);
        vary(color->green);
        vary(color->blue);
    }
    return values;
}
} // namespace sadx::test

extern "C" std::size_t sadx_native_game_view_initial_state(unsigned variant, std::uint32_t *output,
                                                           std::size_t capacity) {
    if (variant > 1 || !output || capacity < 30)
        return 0;
    const auto values =
        variant == 0 ? sadx::game_view_initial_state() : sadx::test::varied_view_initial_state();
    const auto words = sadx::test::view_initial_words(values);
    for (std::size_t i = 0; i < words.size(); ++i)
        output[i] = words[i];
    return words.size();
}
