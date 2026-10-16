#pragma once
#include "engine/game_frame_control.hpp"
#include "game/game_mode.hpp"
#include <cstdint>

namespace sadx::runtime {
// NOTSA: scalar initialization values, separate from live flow/resource ownership.
struct FlowControlDefaults {
    flow::GameMode mode = flow::GameMode::Start;
    std::uint32_t pause_state = 0;
    std::int32_t save_phase = 0;
    std::uint8_t save_ready = 0;
    std::uint8_t action_restart_requested = 0;
    std::uint8_t title_mode = 0;
    std::uint8_t title_load_phase = 0;
    std::uint8_t playing_super_sonic = 0;
    std::uint8_t tutorial_variant = 0;
    std::uint8_t alert_flag = 0;
    std::uint8_t alert_blocked = 0;
    std::uint32_t movie_number = 0;
    std::uint32_t menu_message_enabled = 0;
};
struct RendererControlDefaults {
    std::int64_t counter_frequency = 0;
    std::int32_t frame_interval_multiplier = 1;
    std::int32_t configured_frame_interval = 3;
    std::int64_t frame_interval = 0;
    std::int64_t previous_counter = 0;
};
struct LogicControlDefaults {
    std::int32_t extra_ticks = 0;
    std::int32_t logic_tick_count = 0; // gu32loop_count
    std::int32_t loop_starting = 0;
    std::int32_t demo_playing = 0;
    // Zero/one writes establish a flag cleared on logic entry; its wider meaning is unknown.
    std::uint32_t logic_entry_flag = 0;
    std::int32_t previous_loop_starting = 0;
};
struct FogRangeDefaults {
    float near_distance = 0;
    float far_distance = 0;
};
struct LoopControlDefaults {
    std::uint32_t exit_requested = 0;
    std::uint32_t window_active = 1;
    std::uint8_t loading_file = 0;
    std::uint8_t special_render_flags = 0;
    std::uint8_t fog_profile = 0;
    std::uint8_t queue_fog_enabled = 0;
    FogRangeDefaults active_fog{}, restored_fog{};
    std::uint8_t save_count = 0;
    std::uint8_t save_pending = 0;
    std::uint8_t texture_pending = 0;
    // Nonzero enables the corresponding late-frame/shutdown callee.
    // Broader meanings are not established by this initialization recovery.
    std::uint32_t late_frame_work_enabled = 0;
    std::uint32_t game_frame_count = 0;
    std::uint32_t local_frame_count = 0;
    std::uint32_t shutdown_cleanup_enabled = 0;
    std::uint16_t game_mode = 0; // ssGameMode
    std::int32_t demo_cutscene = 0;
};
struct ResetInputDefaults {
    std::uint8_t soft_reset_flags = 0;
    std::uint8_t reset_input_mask = 0;
};
// NOTSA: load-time control values only. Message resources, sequence targets and
// callbacks stay with their owners; no original pointer is represented here.
struct SessionControlDefaults {
    FlowControlDefaults flow{};
    frame::ModeState frame_mode{};
    RendererControlDefaults renderer{};
    LogicControlDefaults logic{};
    LoopControlDefaults loop{};
    ResetInputDefaults reset_input{};
};
// NOTSA: maintained defaults verified against file bytes and PE zero-filled tails.
SessionControlDefaults session_control_defaults();
} // namespace sadx::runtime
