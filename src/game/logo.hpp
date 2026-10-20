#pragma once
#include "cpu/x87_math.hpp"
#include <cstdint>

namespace sadx::logo {
enum class Mode : std::uint32_t {
    Inactive = 0,
    SegaHold = 1,
    SegaFade = 2,
    Release = 15,
    SonicTeamHold = 16,
    SonicTeamFade = 17
};
enum class Completion : std::uint32_t { Running = 0, Title = 1, Movie = 3 };
// NOTSA: value view of the logo owners; the session or reference adapter owns storage.
struct State {
    Mode mode;
    std::int32_t frames;
    std::uint32_t logic_entry_flag;
    std::uint32_t demo_stage;
};
// NOTSA: the three observed source calls; implementations may change the borrowed state.
struct Calls {
    virtual ~Calls() = default;
    virtual void run_objects() = 0;
    virtual void display(std::uint8_t first_texture) = 0;
    virtual void release_textures() = 0;
};
// FUNCTION: SONIC 0x42cc70 RunSegaSonicTeamLogos
Completion advance(State &, Calls &);
// NOTSA: borrowed display owners; draw callbacks may change these source values.
struct DisplayState {
    std::int32_t &frames;
    float &horizontal_scale, &vertical_scale;
    std::uint32_t &first_vertex_color;
};
struct DisplayCalls {
    virtual ~DisplayCalls() = default;
    virtual void select_logo_textures() = 0;
    virtual void set_material(std::uint32_t) = 0;
    virtual void default_blending() = 0;
    virtual void set_base_color(std::uint32_t) = 0;
    virtual void draw_texture(std::uint32_t, float x, float y, float depth) = 0;
    virtual void draw_box(float width, float height, float depth, std::uint32_t color) = 0;
};
// FUNCTION: SONIC 0x42cb20 DisplayLogoScreen
void display(std::uint8_t first_texture, DisplayState &, DisplayCalls &, X87Context &);
// NOTSA: pinned bgscale wrapper is an identity at 640x480 with unit screen stretch.
// Other viewport/stretch combinations remain explicitly uncomposed.
void display_profile_640(std::uint8_t first_texture, DisplayState &, DisplayCalls &,
                         X87Context &, std::uint32_t width, std::uint32_t height);
} // namespace sadx::logo
