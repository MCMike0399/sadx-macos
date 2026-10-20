#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace sadx { class X87Context; }
namespace sadx::movie {
enum class FadePhase : std::uint32_t { FadeIn, Visible, FadeOut, Black, BeginSkip, Skip };
// Content records are supplied by the asset importer, including signed playlist sentinels.
struct Clip {
    std::string path;
    std::int32_t mode = 0;
    std::int32_t fade_frame = 0;
    std::int16_t width = 0;
    std::int16_t height = 0;
};
struct Content {
    std::array<Clip, 9> clips;
    std::array<std::vector<std::int8_t>, 9> playlists;
};
struct SharedState {
    std::uint32_t &quit;
    std::uint32_t &movie_number;
    std::uint32_t &presented_frames;
};
struct State : SharedState {
    explicit State(SharedState shared) : SharedState(shared) {}
    FadePhase phase = FadePhase::FadeIn;
    std::int32_t clip = 0;
    std::int32_t frame = 0;
    std::int32_t fade_start_frame = 0;
    std::int32_t opacity = 0;
    std::int32_t secondary_opacity = 0;
    std::uint32_t fade_requested = 0;
    std::uint32_t started_milliseconds = 0;
    std::uint32_t elapsed_centiseconds = 0;
    std::uint32_t previous_timer = 0;
    std::uint32_t current_timer = 0;
    std::int32_t timing_counter = 0;
    std::int32_t previous_timing_counter = 0;
    std::uint32_t special_ticks_remaining = 0;
    float fractional_timing = 0;
    std::int32_t configured_width = 0;
    std::int32_t configured_height = 0;
};
// [platform:movie] Media services replace DirectShow; the game retains loop/input/return policy.
struct Services {
    explicit Services(X87Context &context) : fp(context) {}
    X87Context &fp;
    std::function<void()> wait_vsync;
    std::function<void()> release_all_textures;
    std::function<void(float, float)> clip_z;
    std::function<void()> set_camera;
    std::function<void(std::int32_t)> set_wait_vsync_count;
    std::function<void()> enable_pad_reading;
    std::function<void()> poll_input;
    std::function<void()> pump_messages;
    std::function<void()> start_texture_renderer;
    std::function<void()> end_texture_renderer;
    std::function<bool(const std::string &, std::int32_t)> open_clip;
    std::function<void()> start_clip;
    std::function<bool()> clip_running;
    std::function<bool()> texture_ready;
    std::function<void(std::int32_t, std::int32_t)> draw_movie_texture;
    std::function<void(std::uint8_t)> draw_fade;
    std::function<std::uint32_t()> milliseconds;
    std::function<std::uint32_t()> timer;
    std::function<std::uint32_t(std::uint32_t, std::uint32_t)> timer_difference;
    std::function<std::uint32_t(std::uint32_t)> timer_microseconds;
    std::function<std::int32_t()> soft_reset;
    std::function<std::uint32_t()> reset_input_mask;
    std::function<std::int32_t(std::uint32_t)> reset_key;
    std::function<std::uint32_t()> held_buttons;
    std::function<void()> clear_game;
    std::function<void()> reset_game;
    std::function<void()> initialize_special_movie;
    std::function<void()> finish_special_movie;
    std::function<void(std::int32_t)> draw_special_movie;
};
// NOTSA: The WinMain INI subset used by the native movie graph.
std::int32_t music_volume(std::int32_t bgm_enabled, std::int32_t percentage);
// FUNCTION: SONIC 0x42f020 MsMovie
std::int32_t play(State &, const Content &, std::int32_t movie_id,
                  const std::int32_t &music_volume, Services &);
// FUNCTION: SONIC 0x5130a0 movie
std::int32_t run(State &, const Content &, std::int32_t movie_id,
                 const std::int32_t &music_volume, Services &);
struct FadePolygon {
    std::array<std::array<float, 2>, 4> points{};
    std::array<std::uint32_t, 4> colors{};
};
struct FadeDrawServices {
    std::function<void(std::uint32_t, std::uint32_t)> set_blending;
    std::function<void(const FadePolygon &, float, std::uint32_t)> draw_polygon;
};
// FUNCTION: SONIC 0x512d30 DrawFadePoly
void draw_fade_polygon(std::uint8_t opacity, float horizontal_scale, float vertical_scale,
                       FadeDrawServices &, X87Context &);
// FUNCTION: SONIC 0x512df0 FadeControl
bool update_fade(State &, const Content &, std::int32_t movie_id, Services &);
} // namespace sadx::movie
