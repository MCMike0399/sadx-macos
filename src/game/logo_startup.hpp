#pragma once
#include "game/logo.hpp"
#include <cstdint>
#include <string_view>

namespace sadx::logo {
// NOTSA: borrowed startup owners shared with the logo update and demo sequence.
struct StartupState {
    State &logo;
    std::int32_t &demo_mode;
    std::int32_t &demo_event;
    std::int32_t &demo_reset;
};

// NOTSA: native platform/task/resource operations in the original initialization order.
struct StartupCalls {
    virtual ~StartupCalls() = default;
    virtual void initialize_heap() = 0;
    virtual void initialize_tasks() = 0;
    virtual void set_character_selection(std::int32_t) = 0;
    virtual void set_background(std::uint32_t top, std::uint32_t middle,
                                std::uint32_t bottom) = 0;
    virtual void load_textures(std::string_view name) = 0;
    virtual void set_frame_rate(std::int32_t render_ratio, std::int32_t logic_ratio) = 0;
    virtual void set_wait_vsync(std::int32_t) = 0;
    virtual void set_fade_color(std::uint32_t) = 0;
    virtual void initialize_texture_drawing() = 0;
    virtual void initialize_save_data() = 0;
};

// FUNCTION: SONIC 0x42c730 Load_SEGALOGO_E
void load_sega_textures(State &, StartupCalls &);
// FUNCTION: SONIC 0x42c7b0 InitializeSegaLogo
void initialize_sega(StartupState &, StartupCalls &);
} // namespace sadx::logo
