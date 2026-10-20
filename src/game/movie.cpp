#include "game/movie.hpp"
#include "cpu/fidelity_float.hpp"
#include "engine/speedrun.hpp"
#include <stdexcept>

namespace sadx::movie {
// NOTSA: Preserve source 32-bit wrapping counters without signed C++ overflow.
static std::int32_t wrapping_sum(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(left) +
                                     static_cast<std::uint32_t>(right));
}
// NOTSA: Preserve source subtraction before signed fade division.
static std::int32_t wrapping_difference(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(left) -
                                     static_cast<std::uint32_t>(right));
}

std::int32_t music_volume(std::int32_t bgm_enabled, std::int32_t percentage) {
    const auto enabled = bgm_enabled < 0 ? 0 : bgm_enabled > 1 ? 1 : bgm_enabled;
    const auto clamped = percentage <= 0 ? 0 : percentage > 100 ? 100 : percentage;
    const auto volume = ((clamped * 89) / 100 - 100) * 100;
    return enabled == 0 || percentage <= 0 ? -10000 : volume;
}

void draw_fade_polygon(std::uint8_t opacity, float horizontal_scale, float vertical_scale,
                       FadeDrawServices &services, X87Context &fp) {
    using fidelity::ExactFloat;
    const auto height = (ExactFloat(fp, vertical_scale) * 480.0f).rounded();
    const auto width = (ExactFloat(fp, horizontal_scale) * 640.0f).rounded();
    FadePolygon polygon;
    polygon.points = {{{0, 0}, {0, height}, {width, 0}, {width, height}}};
    polygon.colors.fill(static_cast<std::uint32_t>(255 - opacity) << 24);
    services.set_blending(0, 8);
    services.set_blending(1, 6);
    services.draw_polygon(polygon, -1, 96);
}
std::int32_t play(State &state, const Content &content, std::int32_t movie_id,
                  const std::int32_t &music_volume, Services &services) {
    services.wait_vsync();
    services.release_all_textures();
    services.clip_z(-1, -2000);
    services.set_camera();
    services.set_wait_vsync_count(1);
    const auto result = run(state, content, movie_id, music_volume, services);
    state.movie_number = 0;
    services.release_all_textures();
    services.wait_vsync();
    return result;
}
// NOTSA: The pinned Skipma patch consumes held A/Start instead of the original pressed field.
static bool skip_requested(Services &services) {
    return speedrun::pre_rendered_skip_check(services.held_buttons());
}
// NOTSA: Timing block from the movie inner loop, separated to retain its store order.
static void update_movie_timing(State &state, Services &services) {
    using fidelity::ExactFloat;
    auto &fp = services.fp;
    state.previous_timing_counter = state.timing_counter;
    state.current_timer = services.timer();
    const auto difference = services.timer_difference(state.previous_timer, state.current_timer);
    const auto microseconds = services.timer_microseconds(difference);
    auto elapsed = ExactFloat::integer(fp, static_cast<std::int32_t>(microseconds));
    if (static_cast<std::int32_t>(microseconds) < 0)
        elapsed = elapsed + 4294967296.0f;
    const auto sixty_frames = elapsed * 60.0f;
    state.previous_timer = state.current_timer;
    const auto increment = sixty_frames * 0.000001f + state.fractional_timing;
    const auto counter = ExactFloat::integer(fp, state.timing_counter) + increment;
    state.timing_counter = static_cast<std::int32_t>(counter.truncated_integer());
    const auto whole_increment = static_cast<std::int32_t>(increment.truncated_integer());
    state.fractional_timing = (increment - ExactFloat::integer(fp, whole_increment)).rounded();
}
// NOTSA: One source loop iteration, split from playlist traversal without advancing outer game time.
static bool update_clip(State &state, const Content &content, std::int32_t movie_id,
                        std::int32_t &result, Services &services) {
    services.pump_messages();
    services.poll_input();
    state.frame = wrapping_sum(state.frame, 1);
    update_movie_timing(state, services);
    if (services.soft_reset() != 0) {
        if (movie_id != 0) {
            result = -1;
            return false;
        }
        services.clear_game();
        services.reset_game();
        result = -1;
    } else if ((services.reset_input_mask() & 68) != 0 && services.reset_key(61) != 0) {
        state.quit = 1;
    }
    if (skip_requested(services)) {
        result = 0;
        if (state.phase == FadePhase::FadeIn)
            state.fade_requested = 1;
        else if (state.phase == FadePhase::Black)
            return false;
        else if (state.phase == FadePhase::Visible)
            state.phase = FadePhase::BeginSkip;
    }
    if (!services.clip_running())
        return false;
    if (services.texture_ready()) {
        services.draw_movie_texture(state.configured_width, state.configured_height);
        if (movie_id == 7) {
            state.special_ticks_remaining = static_cast<std::uint32_t>(
                wrapping_difference(state.timing_counter, state.previous_timing_counter));
            auto tick = state.previous_timing_counter;
            while (state.special_ticks_remaining != 0) {
                --state.special_ticks_remaining;
                tick = wrapping_sum(tick, 1);
                services.draw_special_movie(tick);
            }
            state.special_ticks_remaining = 0;
        }
        if (update_fade(state, content, movie_id, services))
            return false;
    }
    if (result < 1)
        return false;
    services.wait_vsync();
    ++state.presented_frames;
    return true;
}
std::int32_t run(State &state, const Content &content, std::int32_t movie_id,
                 const std::int32_t &music_volume, Services &services) {
    const auto &configuration = content.clips.at(movie_id);
    const auto &playlist = content.playlists.at(movie_id);
    std::int32_t result = 1;
    services.enable_pad_reading();
    services.poll_input();
    services.start_texture_renderer();
    state.configured_width = configuration.width;
    state.configured_height = configuration.height;
    if (movie_id == 7)
        services.initialize_special_movie();
    std::size_t cursor = 0;
    while (state.quit == 0) {
        state.clip = playlist.at(cursor++);
        if (state.clip == -1)
            break;
        if (state.clip == -2) {
            state.clip = playlist.at(0);
            cursor = 1;
        }
        const auto &clip = content.clips.at(state.clip);
        state.secondary_opacity = 0;
        state.fade_requested = 0;
        state.opacity = 0;
        state.elapsed_centiseconds = 0;
        if (clip.mode != 0)
            services.draw_fade(0);
        state.phase = clip.mode == 0 ? FadePhase::Visible : FadePhase::FadeIn;
        if (!services.open_clip(clip.path, music_volume))
            continue;
        state.started_milliseconds = services.milliseconds();
        services.start_clip();
        state.frame = -1;
        state.previous_timer = services.timer();
        state.fractional_timing = 0;
        state.timing_counter = 0;
        while (state.quit == 0 && update_clip(state, content, movie_id, result, services)) {}
    }
    if (movie_id == 7)
        services.finish_special_movie();
    services.end_texture_renderer();
    return result < 0 ? result : movie_id == 0;
}
// NOTSA: Signed source arithmetic wraps before truncating division.
static std::int32_t fade_product(std::int32_t value, std::int32_t factor) {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(value) *
                                     static_cast<std::uint32_t>(factor));
}
bool update_fade(State &state, const Content &content, std::int32_t movie_id,
                 Services &services) {
    state.elapsed_centiseconds = (services.milliseconds() - state.started_milliseconds) / 10;
    switch (state.phase) {
    case FadePhase::FadeIn:
        state.opacity = fade_product(static_cast<std::int32_t>(state.elapsed_centiseconds), 192) / 100;
        if (state.opacity >= 192) {
            state.opacity = 192;
            state.phase = state.fade_requested == 0 ? FadePhase::Visible : FadePhase::BeginSkip;
        }
        break;
    case FadePhase::Visible:
        state.opacity = 255;
        if (state.frame >= content.clips.at(state.clip).fade_frame) {
            state.fade_start_frame = state.frame;
            state.phase = FadePhase::FadeOut;
        }
        break;
    case FadePhase::FadeOut:
        if (movie_id == 7) {
            state.secondary_opacity = fade_product(wrapping_difference(state.frame, state.fade_start_frame), 255) / 30;
            if (state.secondary_opacity >= 255) {
                state.secondary_opacity = -1;
                state.phase = FadePhase::Black;
            }
        } else {
            const auto remaining = wrapping_sum(
                wrapping_difference(state.fade_start_frame, state.frame), 30);
            state.opacity = fade_product(remaining, 192) /
                            (content.clips.at(state.clip).mode == 2 ? 60 : 30);
            if (state.opacity < 0) {
                state.opacity = 0;
                state.phase = FadePhase::Black;
            }
        }
        break;
    case FadePhase::BeginSkip:
        state.fade_start_frame = state.frame;
        state.phase = FadePhase::Skip;
        [[fallthrough]];
    case FadePhase::Skip:
        if (movie_id == 7) {
            state.secondary_opacity = fade_product(wrapping_difference(state.frame, state.fade_start_frame), 255) / 15;
            if (state.secondary_opacity >= 255) {
                state.secondary_opacity = -1;
                state.phase = FadePhase::Black;
                return true;
            }
            if (state.secondary_opacity < 0)
                return true;
        } else {
            const auto remaining = wrapping_sum(
                wrapping_difference(state.fade_start_frame, state.frame), 15);
            state.opacity = fade_product(remaining, 192) / 15;
            if (state.opacity < 0) {
                state.opacity = 0;
                state.phase = FadePhase::Black;
                return true;
            }
            if (state.opacity == 0)
                return true;
        }
        break;
    case FadePhase::Black:
        break;
    }
    services.draw_fade(static_cast<std::uint8_t>(state.opacity));
    return false;
}
} // namespace sadx::movie
