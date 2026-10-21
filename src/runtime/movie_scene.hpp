#pragma once
#include "game/movie.hpp"
#include "runtime/native_display.hpp"
#include <optional>

namespace sadx::native {
// [platform:movie] Borrowed decoded pixels remain valid until the next media poll.
struct MovieFrameView {
    std::uint32_t width, height;
    const std::uint8_t *bgra;
    std::size_t bytes;
};
// NOTSA: native platform/input owners replace the source media/window interfaces.
// Every callback is required when its corresponding source path executes.
struct MovieHost {
    std::function<void()> present;
    std::function<void()> enable_pad_reading;
    std::function<void()> poll_input;
    std::function<void()> pump_messages;
    std::function<bool(const std::string &, std::int32_t)> open_clip;
    std::function<void()> start_clip, close_clip;
    std::function<bool()> clip_running;
    std::function<std::optional<MovieFrameView>()> decoded_frame;
    std::function<std::uint32_t()> milliseconds, microseconds;
    std::function<std::int32_t()> soft_reset;
    std::function<std::uint32_t()> reset_input_mask, held_buttons;
    std::function<std::int32_t(std::uint32_t)> reset_key;
    std::function<void()> clear_game, reset_game;
};
// NOTSA: connects recovered movie control, owned textures and the shared native display.
class MovieScene {
  public:
    MovieScene(Display &, frame::ModeState &, frame::RendererState &, movie::SharedState,
               const movie::Content &, const std::int32_t &music_volume, MovieHost);
    std::int32_t play(std::uint32_t movie_id);
    const movie::State &state() const { return state_; }
    std::size_t loaded_textures() const { return textures_.entries.size(); }
    bool source_padding_is_defined() const { return false; }
  private:
    Display &display_;
    frame::ModeState &frame_mode_;
    frame::RendererState &renderer_;
    movie::State state_;
    const movie::Content &content_;
    const std::int32_t &music_volume_;
    MovieHost host_;
    movie::Services services_;
    std::shared_ptr<graphics::Texture> texture_;
    std::shared_ptr<std::vector<std::uint8_t>> backing_pixels_;
    texture_lifetime::Memory *texture_memory_ = nullptr;
    sprite::TextureList textures_;
    std::vector<sprite::AnimationFrame> animation_;
    bool texture_ready_ = false;
    void start_renderer();
    void end_renderer();
    bool update_texture();
    // FUNCTION: SONIC 0x5139f0 DrawMovieTex
    void draw_movie_texture(std::int32_t width, std::int32_t height);
    void draw_fade(std::uint8_t opacity);
};
} // namespace sadx::native
