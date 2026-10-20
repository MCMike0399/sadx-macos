#pragma once
#include "game/game_mode.hpp"

namespace sadx::flow {
// NOTSA: session-owned compositions for recovered systems. Missing native systems
// retain their named failing seams; a reference invocation does not install this interface.
struct NativeServices {
    virtual ~NativeServices() = default;
    virtual void initialize_game() = 0;
    virtual void initialize_demo_state() = 0;
    virtual std::uint32_t startup_status() = 0;
    virtual std::int32_t startup_readiness() = 0;
    virtual void initialize_sega_logo() = 0;
    virtual std::uint32_t run_logos() = 0;
    virtual void load_sega_logo() = 0;
    virtual void set_frame_rate_mode(std::int32_t, std::int32_t) = 0;
    virtual void set_wait_vsync_count(std::int32_t) = 0;
    virtual void draw_screen_fade() = 0;
    virtual void increment_frame_counter() = 0;
    virtual std::uint32_t initialize_level_game() = 0;
    virtual void setup_character(std::uint32_t) = 0;
    virtual void reset_player_work() = 0;
    virtual std::int32_t run_movie(std::uint32_t) = 0;
};

// NOTSA: the same dispatcher with native session services.
void advance_game_mode(GameFlowState &, NativeServices &);
} // namespace sadx::flow
