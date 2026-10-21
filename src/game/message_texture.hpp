#pragma once
#include "game/menu_caption.hpp"
#include <cstdint>
namespace sadx::message_texture {
// NOTSA: defined fields of njSetTextureInfo/njSetTextureName for the dynamic
// MSG bitmap. The two uninitialized texture-info words are not read on this
// supported format path.
struct Image {
    const std::uint16_t *pixels;
    std::uint32_t type;
    std::uint32_t width, height;
    std::uint32_t global_index, attributes;
};
struct Calls {
    virtual ~Calls() = default;
    // NOTSA: delegates the recovered native service through its canonical owner.
    // Before this call the pending image describes texaddr's source global ID;
    // entries[0] has no resident native resource until the real loader publishes
    // it.
    virtual std::int32_t load_image(texture_lifetime::List &, const Image &) = 0;
    // NOTSA: delegates the recovered native service through its canonical owner.
    virtual void reload_image(std::uint32_t global_index, const std::uint16_t *pixels,
                              std::uint32_t attributes, std::uint32_t palette) = 0;
};
// NOTSA: the effective loader wrapper preserves the borrowed MipGuard policy byte.
// FUNCTION: SONIC 0x40d2a0 MSG_LoadTexture2
void upload(menu_caption::Context &, std::uint8_t &skip_mipmaps, Calls &);
} // namespace sadx::message_texture
