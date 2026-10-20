#include "game/movie.hpp"
#include "cpu/x87_math.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
namespace {
struct Fixture {
    std::vector<std::uint32_t> calls;
    std::uint32_t pumps = 0;
    std::uint32_t quit = 0;
    std::uint32_t movie = 77;
    std::uint32_t presents = 41;
    sadx::movie::State state{{quit, movie, presents}};
    sadx::movie::Content content;
    sadx::X87Context fp{127};
    sadx::movie::Services services{fp};
    Fixture(const std::uint32_t *in) {
        using namespace sadx::movie;
        for (std::uint32_t id = 0; id < 9; ++id) {
            content.clips[id] = {"clip", id == 5 ? 0 : id == 1 ? 2 : 1,
                                 2147483647, 640, 480};
            content.playlists[id] = {static_cast<std::int8_t>(id), -1};
        }
        auto call = [this](std::uint32_t id) { calls.push_back(id); };
        services.wait_vsync = [=] { call(1); };
        services.release_all_textures = [=] { call(2); };
        services.clip_z = [=](float, float) { call(3); };
        services.set_camera = [=] { call(4); };
        services.set_wait_vsync_count = [=](std::int32_t) { call(5); };
        services.enable_pad_reading = [=] { call(6); };
        services.poll_input = [=] { call(7); };
        services.start_texture_renderer = [=] { call(8); };
        services.end_texture_renderer = [=] { call(9); };
        services.open_clip = [=](const std::string &, std::int32_t volume) {
            call(10);
            calls.push_back(static_cast<std::uint32_t>(volume));
            return in[5] == 0;
        };
        services.start_clip = [=] { call(11); };
        services.milliseconds = [] { return 1234; };
        services.timer = [] { return 0; };
        services.timer_difference = [](std::uint32_t, std::uint32_t) { return 0; };
        services.timer_microseconds = [](std::uint32_t) { return 0; };
        services.pump_messages = [=] {
            call(12);
            ++pumps;
            if (in[6] != 0 && pumps >= in[6])
                quit = 1;
        };
        services.soft_reset = [=] {
            call(13);
            return in[3] != 0 && pumps >= in[3] ? 1 : 0;
        };
        services.reset_input_mask = [=] { call(14); return 0; };
        services.reset_key = [](std::uint32_t) { return 0; };
        services.held_buttons = [=] { return pumps >= in[2] ? in[1] : 0; };
        services.clip_running = [=] { call(15); return pumps < in[4]; };
        services.texture_ready = [=] { call(16); return false; };
        services.draw_fade = [=](std::uint8_t alpha) { call(17); calls.push_back(alpha); };
        services.clear_game = [=] { call(18); };
        services.reset_game = [=] { call(19); };
        services.initialize_special_movie = [=] { call(20); };
        services.finish_special_movie = [=] { call(21); };
    }
};
}
extern "C" void sadx_native_movie(const std::uint32_t *in, std::uint32_t *out) {
    Fixture fixture(in);
    const std::int32_t volume = static_cast<std::int32_t>(in[7]);
    const auto result = sadx::movie::play(fixture.state, fixture.content,
                                         static_cast<std::int32_t>(in[0]), volume,
                                         fixture.services);
    std::fill_n(out, 1024, 0);
    const std::uint32_t values[]{static_cast<std::uint32_t>(result), fixture.quit,
                                 fixture.movie, fixture.presents,
                                 static_cast<std::uint32_t>(fixture.state.phase),
                                 fixture.state.fade_requested,
                                 static_cast<std::uint32_t>(fixture.state.frame),
                                 static_cast<std::uint32_t>(fixture.calls.size())};
    std::copy(std::begin(values), std::end(values), out);
    std::copy(fixture.calls.begin(), fixture.calls.end(), out + 8);
}
extern "C" void sadx_native_movie_fade(const std::uint32_t *in, std::uint32_t *out) {
    using namespace sadx::movie;
    std::uint32_t quit = 0, number = 0, frames = 0;
    State state({quit, number, frames});
    state.phase = static_cast<FadePhase>(in[0]);
    state.frame = static_cast<std::int32_t>(in[1]);
    state.fade_start_frame = static_cast<std::int32_t>(in[2]);
    state.clip = 0;
    state.opacity = static_cast<std::int32_t>(in[5]);
    state.secondary_opacity = static_cast<std::int32_t>(in[6]);
    state.fade_requested = in[7];
    state.started_milliseconds = in[9];
    Content content;
    content.clips[0].mode = static_cast<std::int32_t>(in[3]);
    content.clips[0].fade_frame = static_cast<std::int32_t>(in[4]);
    sadx::X87Context fp{127};
    Services services(fp);
    services.milliseconds = [=] { return in[10]; };
    std::uint32_t drawn = 0, alpha = 0;
    services.draw_fade = [&](std::uint8_t value) { ++drawn; alpha = value; };
    const auto result = update_fade(state, content, static_cast<std::int32_t>(in[8]), services);
    const std::uint32_t values[]{static_cast<std::uint32_t>(state.phase),
                                 static_cast<std::uint32_t>(state.opacity),
                                 static_cast<std::uint32_t>(state.secondary_opacity),
                                 static_cast<std::uint32_t>(state.fade_start_frame),
                                 state.elapsed_centiseconds, result, drawn, alpha};
    std::copy(std::begin(values), std::end(values), out);
}
extern "C" std::int32_t sadx_native_movie_volume(std::int32_t enabled, std::int32_t percentage) {
    return sadx::movie::music_volume(enabled, percentage);
}
extern "C" void sadx_native_movie_fade_polygon(const std::uint32_t *in, std::uint32_t *out) {
    using namespace sadx::movie;
    float horizontal, vertical;
    std::memcpy(&horizontal, in + 1, 4);
    std::memcpy(&vertical, in + 2, 4);
    sadx::X87Context fp(static_cast<std::uint16_t>(in[3]));
    FadeDrawServices services;
    std::uint32_t blend = 12;
    services.set_blending = [&](std::uint32_t selector, std::uint32_t mode) {
        out[blend++] = selector;
        out[blend++] = mode;
    };
    services.draw_polygon = [&](const FadePolygon &polygon, float depth, std::uint32_t attr) {
        std::memcpy(out, polygon.points.data(), 32);
        std::copy(polygon.colors.begin(), polygon.colors.end(), out + 8);
        std::memcpy(out + 16, &depth, 4);
        out[17] = 4;
        out[18] = attr;
    };
    draw_fade_polygon(static_cast<std::uint8_t>(in[0]), horizontal, vertical, services, fp);
    out[19] = fp.exception_flags();
}
extern "C" void sadx_native_movie_timing(const std::uint32_t *in, std::uint32_t *out) {
    const std::uint32_t control[]{0, 0, 0, 0, 5, 0, 0, 4294959896U};
    Fixture fixture(control);
    fixture.fp.set_control_word(static_cast<std::uint16_t>(in[0]));
    std::uint32_t timer = in[1];
    fixture.services.timer = [&] { timer += in[2]; return timer; };
    fixture.services.timer_difference = [](std::uint32_t old, std::uint32_t now) { return now - old; };
    fixture.services.timer_microseconds = [](std::uint32_t value) { return value; };
    const std::int32_t volume = -7400;
    sadx::movie::play(fixture.state, fixture.content, 0, volume, fixture.services);
    const auto &state = fixture.state;
    std::uint32_t fraction;
    std::memcpy(&fraction, &state.fractional_timing, 4);
    const std::uint32_t values[]{state.previous_timer, state.current_timer,
                                 static_cast<std::uint32_t>(state.timing_counter),
                                 static_cast<std::uint32_t>(state.previous_timing_counter), fraction,
                                 fixture.fp.exception_flags()};
    std::copy(std::begin(values), std::end(values), out);
}
