#include "runtime/movie_scene.hpp"
#include "cpu/fidelity_float.hpp"
#include "game/game_mode.hpp"
#include <algorithm>
#include <stdexcept>

namespace sadx::native {
MovieScene::MovieScene(Display &display, frame::ModeState &mode, frame::RendererState &renderer,
                       movie::SharedState shared, const movie::Content &content,
                       const std::int32_t &volume, MovieHost host)
    : display_(display), frame_mode_(mode), renderer_(renderer), state_(shared), content_(content),
      music_volume_(volume), host_(std::move(host)), services_(display.arithmetic()) {
    services_.wait_vsync = host_.present;
    services_.release_all_textures = [this] {
        display_.release_all_textures();
    };
    services_.clip_z = [this](float near_distance, float far_distance) {
        display_.set_clip_range(near_distance, far_distance);
    };
    services_.set_camera = [this] {
        display_.set_camera();
    };
    services_.set_wait_vsync_count = [this](std::int32_t count) {
        frame::set_frame_interval_multiplier(count, renderer_);
    };
    services_.enable_pad_reading = host_.enable_pad_reading;
    services_.poll_input = host_.poll_input;
    services_.pump_messages = host_.pump_messages;
    services_.start_texture_renderer = [this] {
        start_renderer();
    };
    services_.end_texture_renderer = [this] {
        end_renderer();
    };
    services_.open_clip = host_.open_clip;
    services_.start_clip = host_.start_clip;
    services_.clip_running = host_.clip_running;
    services_.texture_ready = [this] {
        return update_texture();
    };
    services_.draw_movie_texture = [this](std::int32_t width, std::int32_t height) {
        draw_movie_texture(width, height);
    };
    services_.draw_fade = [this](std::uint8_t opacity) {
        draw_fade(opacity);
    };
    services_.milliseconds = host_.milliseconds;
    services_.timer = host_.microseconds;
    services_.timer_difference = [](std::uint32_t old, std::uint32_t current) {
        return current - old;
    };
    services_.timer_microseconds = [](std::uint32_t elapsed) {
        return elapsed;
    };
    services_.soft_reset = host_.soft_reset;
    services_.reset_input_mask = host_.reset_input_mask;
    services_.reset_key = host_.reset_key;
    services_.held_buttons = host_.held_buttons;
    services_.clear_game = host_.clear_game;
    services_.reset_game = host_.reset_game;
    services_.initialize_special_movie = [] {
        throw flow::UnrecoveredGameFlowCall("initialize_special_movie");
    };
    services_.finish_special_movie = [] {
        throw flow::UnrecoveredGameFlowCall("finish_special_movie");
    };
    services_.draw_special_movie = [](std::int32_t) {
        throw flow::UnrecoveredGameFlowCall("draw_special_movie");
    };
}
std::int32_t MovieScene::play(std::uint32_t movie_id) {
    return movie::play(state_, content_, static_cast<std::int32_t>(movie_id), music_volume_, services_);
}
void MovieScene::start_renderer() {
    if (!texture_) {
        texture_ = std::make_shared<graphics::Texture>();
        texture_->width = 1024;
        texture_->height = 1024;
        texture_->format = graphics::TextureFormat::Bgra8;
        // [platform:movie] Source HeapAlloc leaves padding and alpha unspecified.
        // Native storage clears these bytes explicitly; only uploaded RGB is source-defined.
        // The source UV range can sample padding, so those pixels are not a parity claim.
        backing_pixels_ = std::make_shared<std::vector<std::uint8_t>>(1024 * 1024 * 4, 0);
        texture_->bgra = *backing_pixels_;
        constexpr std::uint32_t kMovieGlobalTextureIndex = 3489661136U;
        texture_memory_ =
            display_.create_argb_texture(kMovieGlobalTextureIndex, backing_pixels_, texture_);
        textures_.entries = {texture_memory_};
    }
    // RendererStart uses fixed256 UV units; Sprite2D subsequently scales by1/255.
    animation_ = {{640, 480, 0, 0, 0, 0, 160, 120, 0, 0}};
    texture_ready_ = false;
}
void MovieScene::end_renderer() {
    host_.close_clip();
    if (backing_pixels_) {
        backing_pixels_.reset();
        display_.release_movie_texture_list(textures_);
    }
    textures_.entries.clear();
    texture_memory_ = nullptr;
    texture_.reset();
    texture_ready_ = false;
}
bool MovieScene::update_texture() {
    const auto decoded = host_.decoded_frame();
    if (decoded) {
        if (!texture_ || !backing_pixels_ || !texture_memory_ || !decoded->bgra ||
            decoded->width > 1024 || decoded->height > 1024 ||
            decoded->width == 0 || decoded->height == 0 ||
            decoded->bytes != std::size_t(decoded->width) * decoded->height * 4)
            throw std::runtime_error("native movie surface requires supported decoded dimensions");
        for (std::uint32_t row = 0; row < decoded->height; ++row)
            for (std::uint32_t column = 0; column < decoded->width; ++column) {
                const auto source = (std::size_t(row) * decoded->width + column) * 4;
                const auto target = (std::size_t(row) * 1024 + column) * 4;
                std::copy_n(decoded->bgra + source, 3, backing_pixels_->data() + target);
            }
        auto updated = std::make_shared<graphics::Texture>();
        updated->width = texture_->width;
        updated->height = texture_->height;
        updated->format = texture_->format;
        updated->bgra = *backing_pixels_;
        texture_ = std::move(updated);
        texture_lifetime::update_surface(*texture_memory_, texture_);
        texture_ready_ = true;
    }
    // GetNJTexture retains its ready bit: redraw the latest decoded surface every game present.
    return texture_ready_;
}
void MovieScene::draw_movie_texture(std::int32_t width, std::int32_t height) {
    using fidelity::ExactFloat;
    auto &fp = display_.arithmetic();
    const auto scaling = display_.frame_state();
    display_.set_constant_material({1, 1, 1, 1});
    const auto horizontal_ratio = 640.0f / ExactFloat::integer(fp, width);
    animation_.at(0).center_x = static_cast<std::int16_t>(width / 2);
    animation_.at(0).center_y = static_cast<std::int16_t>(height / 2);
    const auto horizontal = (horizontal_ratio * scaling.horizontal_scale).rounded();
    const auto vertical = ((480.0f / ExactFloat::integer(fp, height)) * scaling.vertical_scale).rounded();
    const auto x = (ExactFloat(fp, scaling.horizontal_scale) * 320.0f).rounded();
    const auto y = (ExactFloat(fp, scaling.vertical_scale) * 240.0f).rounded();
    sprite::Sprite value{{x, y, 0}, horizontal, vertical, 0, &textures_, animation_};
    display_.draw_sprite(value, 0, -1000, 2);
}
void MovieScene::draw_fade(std::uint8_t opacity) {
    const auto scaling = display_.frame_state();
    movie::FadeDrawServices services{
        [this](std::uint32_t side, std::uint32_t mode) { display_.output().set_blending(side, mode); },
        [this](const movie::FadePolygon &polygon, float priority, std::uint32_t attributes) {
            // __njDrawPolygon2D: this nonnegative-attribute path has no texture readiness gates.
            if (frame_mode_.pending_logic_ticks != 0)
                return;
            display_.set_depth_comparison(7);
            display_.set_depth_write(false);
            std::array<pvr::Point, 4> points;
            for (std::size_t i = 0; i < points.size(); ++i) {
                if (polygon.colors[i] != polygon.colors[0])
                    throw std::runtime_error("native movie fade requires its source uniform color");
                points[i] = {polygon.points[i][0], polygon.points[i][1]};
            }
            display_.output().draw_polygon(points, polygon.colors[0], priority, attributes);
            display_.set_depth_write(true);
            display_.restore_depth_comparison();
        }};
    movie::draw_fade_polygon(opacity, scaling.horizontal_scale, scaling.vertical_scale,
                             services, display_.arithmetic());
}
} // namespace sadx::native
