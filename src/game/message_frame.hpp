#pragma once
#include "cpu/x87_math.hpp"
#include "engine/sprite_immediate.hpp"
#include "game/menu_caption.hpp"
#include <stdexcept>
namespace sadx::message_frame {
// NOTSA: defined fields copied by the MSG source caller. Source z/angle stack
// words are indeterminate and neither is read by its attributes34 sprite call.
struct SpriteInput {
    float x, y, scale_x, scale_y;
    const texture_lifetime::List *textures;
    const sprite::AnimationFrame *animation;
};
struct StateView {
    cleanup::MessageState &queue;
    std::int32_t &saved_fov;
    const float &horizontal_scale, &vertical_scale;
};
struct Calls {
    virtual ~Calls() = default;
    // NOTSA: resolve only actual known Context identities; unknown queued kinds
    // fail.
    virtual menu_caption::Context &resolve(const cleanup::Message *) = 0;
    virtual std::uint32_t current_fov() = 0;
    virtual void set_fov(std::uint32_t) = 0;
    virtual void set_depth_comparison(std::uint32_t) = 0;
    virtual void set_depth_write(bool) = 0;
    virtual void default_blending() = 0;
    virtual void set_blend(std::uint32_t side, std::uint32_t mode) = 0;
    virtual void set_panel_color(std::uint32_t) = 0;
    virtual void draw_panel(float x, float y, float depth, float width, float height) = 0;
    virtual void draw_box(float left, float top, float right, float bottom, float priority,
                          std::uint32_t color) = 0;
    virtual void upload_texture(menu_caption::Context &) = 0;
    // STUB: SONIC 0x403d30 late_alloca
    virtual sprite::AnimationFrame &allocate_animation(std::uint32_t bytes) = 0;
    virtual void set_material(const std::array<float, 4> &) = 0;
    // NOTSA: delegates the recovered native service through its canonical owner.
    virtual void publish_centered_sprite(const SpriteInput &) = 0;
    virtual void set_sprite_y(float) = 0;
    // NOTSA: delegates the recovered native service through its canonical owner.
    virtual void submit_sprite(std::int32_t frame, float priority, std::uint32_t attributes,
                               std::uint32_t queue_flags) = 0;
    virtual void complete_caption_frame() = 0;
};
enum class Boundary {
    // Source40d520 indexes the fixed four-context queue.
    QueueStorage
};
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, const char *reason)
        : std::runtime_error(reason), boundary_(boundary) {}
    Boundary boundary() const { return boundary_; }

  private:
    Boundary boundary_;
};
// FUNCTION: SONIC 0x40d4d0 DrawSubtitles
void render(StateView, Calls &, X87Context &);
} // namespace sadx::message_frame
