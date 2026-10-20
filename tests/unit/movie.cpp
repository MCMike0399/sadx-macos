#include "game/movie.hpp"
#include "cpu/x87_math.hpp"
#include <cassert>
#include <vector>

int main() {
    using namespace sadx::movie;
    assert(music_volume(1, 30) == -7400);
    assert(music_volume(0, 100) == -10000);
    assert(music_volume(1, 0) == -10000);
    assert(music_volume(1, 100) == -1100);
    assert(music_volume(99, 200) == -1100);
    assert(music_volume(-1, 30) == -10000);
    std::uint32_t quit = 0, movie_number = 17, presented_frames = 0;
    State state({quit, movie_number, presented_frames});
    Content content;
    content.clips[0] = {"intro", 1, 2147483647, 640, 480};
    content.playlists[0] = {0, -1};
    sadx::X87Context fp(127);
    Services services(fp);
    std::vector<unsigned> lifecycle;
    unsigned pumps = 0, waits = 0;
    std::uint32_t timer = 0;
    services.wait_vsync = [&] { ++waits; };
    services.release_all_textures = [&] { lifecycle.push_back(1); };
    services.clip_z = [&](float near, float far) {
        assert(near == -1 && far == -2000);
        lifecycle.push_back(2);
    };
    services.set_camera = [&] { lifecycle.push_back(3); };
    services.set_wait_vsync_count = [](std::int32_t count) { assert(count == 1); };
    services.enable_pad_reading = [] {};
    services.poll_input = [] {};
    services.pump_messages = [&] { ++pumps; };
    services.start_texture_renderer = [&] { lifecycle.push_back(4); };
    services.end_texture_renderer = [&] { lifecycle.push_back(6); };
    services.open_clip = [&](const std::string &path, std::int32_t volume) {
        assert(path == "intro" && volume == -7400);
        lifecycle.push_back(5);
        return true;
    };
    services.start_clip = [] {};
    services.clip_running = [&] {
        // The source publishes its held-skip request before querying completion.
        if (pumps == 2)
            assert(state.fade_requested == 1);
        return true;
    };
    services.texture_ready = [] { return false; };
    services.draw_fade = [](std::uint8_t opacity) { assert(opacity == 0); };
    services.milliseconds = [] { return 10; };
    services.timer = [&] { timer += 16667; return timer; };
    services.timer_difference = [](std::uint32_t old, std::uint32_t now) { return now - old; };
    services.timer_microseconds = [](std::uint32_t value) { return value; };
    services.soft_reset = [] { return 0; };
    services.reset_input_mask = [] { return 0; };
    services.held_buttons = [&] { return pumps == 2 ? 8 : 0; };
    const std::int32_t volume = -7400;
    assert(play(state, content, 0, volume, services) == 1);
    assert(pumps == 2 && waits == 3 && presented_frames == 1);
    assert(movie_number == 0 && quit == 0);
    assert(state.timing_counter == 2 && state.previous_timing_counter == 1);
    assert((lifecycle == std::vector<unsigned>{1, 2, 3, 4, 5, 6, 1}));

    FadeDrawServices drawing;
    std::vector<std::uint32_t> blending;
    drawing.set_blending = [&](std::uint32_t side, std::uint32_t mode) {
        blending.push_back(side);
        blending.push_back(mode);
    };
    drawing.draw_polygon = [&](const FadePolygon &polygon, float priority, std::uint32_t flags) {
        assert((blending == std::vector<std::uint32_t>{0, 8, 1, 6}));
        assert(priority == -1 && flags == 96);
        assert(polygon.points[3][0] == 640 && polygon.points[3][1] == 480);
        for (const auto color : polygon.colors)
            assert(color == 1056964608U); // opacity192 leaves black alpha63.
    };
    draw_fade_polygon(192, 1, 1, drawing, fp);
}
