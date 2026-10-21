#include "game/message_texture.hpp"
namespace sadx::message_texture {
namespace {
// NOTSA: effective loader MipGuard(true), including its restoration on nested
// calls.
class MipmapScope {
  public:
    explicit MipmapScope(std::uint8_t &policy) : policy_(policy), previous_(policy) { policy_ = 1; }
    ~MipmapScope() { policy_ = previous_; }

  private:
    std::uint8_t &policy_;
    std::uint8_t previous_;
};
} // namespace
void upload(menu_caption::Context &context, std::uint8_t &skip_mipmaps, Calls &calls) {
    const MipmapScope guard(skip_mipmaps);
    if (!context.bitmap)
        return;
    if (!context.texture_names_attached) {
        const Image image{context.bitmap,       2306,
                          context.bitmap_width, context.bitmap_height,
                          context.global_index, 1082130432};
        // Source publishes one pending texname before calling its loader. A null
        // native resource means not yet resident; its source ID is in image above.
        context.textures.entries.assign(1, nullptr);
        context.textures.attributes.assign(1, image.attributes);
        context.texture_names_attached = true;
        if (calls.load_image(context.textures, image) < 0) {
            context.texture_names_attached = false;
            return;
        }
    } else {
        calls.reload_image(context.global_index, context.bitmap, 1082130432, 0);
    }
    context.flags &= ~2U;
}
} // namespace sadx::message_texture
