#include "session_control_probe.hpp"
#include <cstddef>
#include <cstring>
#include <type_traits>

namespace sadx::test {
std::array<std::uint32_t, 51> control_words(const runtime::SessionControlDefaults &values) {
    const auto real_bits = [](float value) {
        std::uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        return bits;
    };
    return {
        static_cast<std::uint32_t>(values.flow.mode),
        static_cast<std::uint32_t>(values.flow.pause_state),
        static_cast<std::uint32_t>(values.flow.save_phase),
        static_cast<std::uint32_t>(values.flow.save_ready),
        static_cast<std::uint32_t>(values.flow.action_restart_requested),
        static_cast<std::uint32_t>(values.flow.title_mode),
        static_cast<std::uint32_t>(values.flow.title_load_phase),
        static_cast<std::uint32_t>(values.flow.playing_super_sonic),
        static_cast<std::uint32_t>(values.flow.tutorial_variant),
        static_cast<std::uint32_t>(values.flow.alert_flag),
        static_cast<std::uint32_t>(values.flow.alert_blocked),
        static_cast<std::uint32_t>(values.flow.movie_number),
        static_cast<std::uint32_t>(values.flow.menu_message_enabled),
        static_cast<std::uint32_t>(values.frame_mode.frame_multiplier),
        static_cast<std::uint32_t>(values.frame_mode.logic_increment),
        static_cast<std::uint32_t>(values.frame_mode.pending_logic_ticks),
        static_cast<std::uint32_t>(std::uint64_t(values.renderer.counter_frequency)),
        static_cast<std::uint32_t>(std::uint64_t(values.renderer.counter_frequency) >> 32),
        static_cast<std::uint32_t>(std::uint64_t(values.renderer.frame_interval)),
        static_cast<std::uint32_t>(std::uint64_t(values.renderer.frame_interval) >> 32),
        static_cast<std::uint32_t>(values.renderer.frame_interval_multiplier),
        static_cast<std::uint32_t>(values.renderer.configured_frame_interval),
        static_cast<std::uint32_t>(std::uint64_t(values.renderer.previous_counter)),
        static_cast<std::uint32_t>(std::uint64_t(values.renderer.previous_counter) >> 32),
        static_cast<std::uint32_t>(values.logic.extra_ticks),
        static_cast<std::uint32_t>(values.logic.logic_tick_count),
        static_cast<std::uint32_t>(values.logic.loop_starting),
        static_cast<std::uint32_t>(values.logic.demo_playing),
        static_cast<std::uint32_t>(values.logic.logic_entry_flag),
        static_cast<std::uint32_t>(values.logic.previous_loop_starting),
        static_cast<std::uint32_t>(values.loop.exit_requested),
        static_cast<std::uint32_t>(values.loop.window_active),
        static_cast<std::uint32_t>(values.loop.loading_file),
        static_cast<std::uint32_t>(values.loop.special_render_flags),
        static_cast<std::uint32_t>(values.loop.fog_profile),
        static_cast<std::uint32_t>(values.loop.queue_fog_enabled),
        real_bits(values.loop.active_fog.near_distance),
        real_bits(values.loop.active_fog.far_distance),
        real_bits(values.loop.restored_fog.near_distance),
        real_bits(values.loop.restored_fog.far_distance),
        static_cast<std::uint32_t>(values.loop.save_count),
        static_cast<std::uint32_t>(values.loop.save_pending),
        static_cast<std::uint32_t>(values.loop.texture_pending),
        static_cast<std::uint32_t>(values.loop.late_frame_work_enabled),
        static_cast<std::uint32_t>(values.loop.game_frame_count),
        static_cast<std::uint32_t>(values.loop.local_frame_count),
        static_cast<std::uint32_t>(values.loop.shutdown_cleanup_enabled),
        static_cast<std::uint32_t>(values.loop.game_mode),
        static_cast<std::uint32_t>(values.loop.demo_cutscene),
        static_cast<std::uint32_t>(values.reset_input.soft_reset_flags),
        static_cast<std::uint32_t>(values.reset_input.reset_input_mask),
    };
}
runtime::SessionControlDefaults varied_control_defaults() {
    auto values = runtime::session_control_defaults();
    std::uint32_t ordinal = 1;
    const auto vary = [&](auto &value) {
        using T = std::decay_t<decltype(value)>;
        const auto index = ordinal++;
        if constexpr (std::is_floating_point_v<T>)
            value = static_cast<T>(index) + 0.25f;
        else if constexpr (std::is_enum_v<T>)
            value = static_cast<T>(index);
        else if constexpr (sizeof(T) == 8)
            value = static_cast<T>((std::uint64_t(index) << 32) | (index * 1234U));
        else if constexpr (std::is_signed_v<T>)
            value = -static_cast<T>(index);
        else
            value = static_cast<T>(index);
    };
    vary(values.flow.mode);
    vary(values.flow.pause_state);
    vary(values.flow.save_phase);
    vary(values.flow.save_ready);
    vary(values.flow.action_restart_requested);
    vary(values.flow.title_mode);
    vary(values.flow.title_load_phase);
    vary(values.flow.playing_super_sonic);
    vary(values.flow.tutorial_variant);
    vary(values.flow.alert_flag);
    vary(values.flow.alert_blocked);
    vary(values.flow.movie_number);
    vary(values.flow.menu_message_enabled);
    vary(values.frame_mode.frame_multiplier);
    vary(values.frame_mode.logic_increment);
    vary(values.frame_mode.pending_logic_ticks);
    vary(values.renderer.counter_frequency);
    vary(values.renderer.frame_interval);
    vary(values.renderer.frame_interval_multiplier);
    vary(values.renderer.configured_frame_interval);
    vary(values.renderer.previous_counter);
    vary(values.logic.extra_ticks);
    vary(values.logic.logic_tick_count);
    vary(values.logic.loop_starting);
    vary(values.logic.demo_playing);
    vary(values.logic.logic_entry_flag);
    vary(values.logic.previous_loop_starting);
    vary(values.loop.exit_requested);
    vary(values.loop.window_active);
    vary(values.loop.loading_file);
    vary(values.loop.special_render_flags);
    vary(values.loop.fog_profile);
    vary(values.loop.queue_fog_enabled);
    vary(values.loop.active_fog.near_distance);
    vary(values.loop.active_fog.far_distance);
    vary(values.loop.restored_fog.near_distance);
    vary(values.loop.restored_fog.far_distance);
    vary(values.loop.save_count);
    vary(values.loop.save_pending);
    vary(values.loop.texture_pending);
    vary(values.loop.late_frame_work_enabled);
    vary(values.loop.game_frame_count);
    vary(values.loop.local_frame_count);
    vary(values.loop.shutdown_cleanup_enabled);
    vary(values.loop.game_mode);
    vary(values.loop.demo_cutscene);
    vary(values.reset_input.soft_reset_flags);
    vary(values.reset_input.reset_input_mask);
    return values;
}
} // namespace sadx::test

extern "C" std::size_t sadx_native_session_control_defaults(unsigned variant, std::uint32_t *output,
                                                            std::size_t capacity) {
    if (variant > 1 || !output || capacity < 51)
        return 0;
    const auto values = variant == 0 ? sadx::runtime::session_control_defaults()
                                     : sadx::test::varied_control_defaults();
    const auto words = sadx::test::control_words(values);
    for (std::size_t i = 0; i < words.size(); ++i)
        output[i] = words[i];
    return words.size();
}
