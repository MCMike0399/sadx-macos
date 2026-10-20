#include "runtime/movie_scene.hpp"
#include <cassert>
#include <iostream>

int main() {
    using namespace sadx;
    native::Display display(".");
    frame::ModeState mode{1, 1, 0};
    frame::RendererState renderer;
    std::uint32_t quit = 0, movie_number = 77, completed = 388;
    movie::Content content{};
    content.clips[0] = {"fixture.mpg", 1, INT32_MAX, 640, 480};
    content.playlists[0] = {0, -1};
    const std::int32_t volume = -7400;
    unsigned presents = 0, polls = 0, starts = 0, closes = 0, movie_draws = 0;
    std::vector<std::uint8_t> pixels(640 * 480 * 4, 255);
    native::MovieHost host;
    host.present = [&] {
        ++presents;
        for (const auto &command : display.output().frame().commands) {
            const auto *draw = std::get_if<graphics::Draw>(&command);
            if (!draw || !draw->texture)
                continue;
            ++movie_draws;
            assert(draw->texture->width == 1024 && draw->texture->height == 1024);
            assert(draw->texture->bgra[0] == 255 && draw->texture->bgra[3] == 0);
            assert(draw->render_states.at(27) == 0 && draw->stage0_states.at(4) == 3);
            assert(draw->stage0_states.at(16) == 1);
            assert(draw->vertices.size() == 4 && draw->vertices[0].position[0] == 0.5f);
            assert(draw->vertices[0].position[1] == 0.5f && draw->vertices[0].diffuse[3] == 1);
            // Original Sprite2D with the movie frame produces words3f20a0a1/3ef0f0f2.
            assert(draw->vertices[3].uv[0][0] == 0.6274510025978088f);
            assert(draw->vertices[3].uv[0][1] == 0.4705882668495178f);
        }
        display.output().begin_frame();
    };
    host.enable_pad_reading = [] {};
    host.poll_input = [] {};
    host.pump_messages = [] {};
    host.open_clip = [&](const std::string &path, std::int32_t actual_volume) {
        assert(path == "fixture.mpg" && actual_volume == volume);
        return true;
    };
    host.start_clip = [&] { ++starts; };
    host.close_clip = [&] { ++closes; };
    host.clip_running = [&] { return ++polls <= 3; };
    host.decoded_frame = [&]() -> std::optional<native::MovieFrameView> {
        if (polls != 1)
            return std::nullopt;
        return native::MovieFrameView{640, 480, pixels.data(), pixels.size()};
    };
    host.milliseconds = [&] { return presents * 16; };
    host.microseconds = [&] { return presents * 16000; };
    host.soft_reset = [] { return 0; };
    host.reset_input_mask = [] { return 0; };
    host.reset_key = [](std::uint32_t) { return 0; };
    host.held_buttons = [] { return 0; };
    host.clear_game = [] { assert(false); };
    host.reset_game = [] { assert(false); };
    native::MovieScene scene(display, mode, renderer, {quit, movie_number, completed}, content,
                            volume, std::move(host));
    display.output().begin_frame();
    assert(scene.play(0) == 1);
    assert(presents == 5 && completed == 391 && movie_number == 0 && quit == 0);
    assert(starts == 1 && closes == 1 && movie_draws == 3);
    assert(scene.loaded_textures() == 0 && !scene.source_padding_is_defined());
    std::cout << "Native movie scene: three redraws from one decoded frame, exact source UV/alpha "
                 "path, live volume, wrapper/counter order and resource release passed.\n";
}
