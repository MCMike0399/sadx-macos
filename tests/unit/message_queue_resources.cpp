#include "engine/message_sprite_queue.hpp"
#include "engine/queue_consumer_native.hpp"
#include <cassert>
#include <cstring>
#include <vector>
int main() {
    using namespace sadx;
    queue::Storage storage;
    storage.bind_buffer(queue::allocate_buffer());
    auto arena = storage.buffer().arena;
    const auto cursor = arena->cursor(), end = arena->end();
    auto bitmap = queue::borrow_from_end(storage, 0x20020);
    assert(bitmap.owner == arena && bitmap.size == 0x20020 &&
           bitmap.bytes == arena->bytes().data() + end - 0x20020);
    assert(arena->cursor() == cursor && arena->end() == end);
    std::memset(bitmap.bytes, 0x55, bitmap.size);
    const auto repeat = queue::borrow_from_end(storage, 0x20020);
    assert(repeat.bytes == bitmap.bytes);
    auto animation = queue::allocate_animation(storage, 20, {});
    assert(animation.storage.owner == arena &&
           reinterpret_cast<std::uint8_t *>(animation.frame) == animation.storage.bytes);
    *animation.frame = {256, 128, 0, 0, 0, 0, 255, 255, 0, 0};
    const auto *frame_identity = animation.frame;
    const auto *bitmap_identity = bitmap.bytes;
    storage.reset_buffer();
    assert(bitmap.bytes == bitmap_identity && animation.frame == frame_identity);
    for (auto byte : arena->bytes())
        assert(byte == 0);
    assert(animation.frame->width == 0 && animation.frame->height == 0);
    sprite::ScreenSpriteScratch scratch;
    sprite::ScreenSprite source;
    source.x = 11;
    source.y = 22;
    source.scale_x = 1;
    source.scale_y = 1;
    source.animation = animation.frame;
    source.animation_count = 1;
    source.animation_owner = arena;
    float hs = 1, vs = 1;
    X87Context fp(127);
    sprite::center_screen_sprite(scratch, source, hs, vs, fp);
    assert(scratch.value.x == 11 && scratch.value.y == 22 && !scratch.value.angle &&
           !scratch.value.copied_z_bits);
    sprite::TextureList textures;
    scratch.value.textures = &textures;
    auto &frame = *animation.frame;
    frame = {20, 10, 0, 0, 0, 0, 255, 255, 0, 0};
    sprite::Quad captured{};
    int texture = -1, angles = 0, pushes = 0;
    sprite::ImmediateServices services{[&](const sprite::TextureList &) {
                                           scratch.value.x = 10;
                                           scratch.value.y = 20;
                                           scratch.value.scale_x = 2;
                                           scratch.value.scale_y = 3;
                                           frame.width = 30;
                                       },
                                       [&] { ++pushes; },
                                       [&](std::int32_t) { ++angles; },
                                       [](const std::array<float, 3> &value) { return value; },
                                       [] {},
                                       [&](bool alpha) {
                                           assert(alpha);
                                           frame.texture_index = 7;
                                           scratch.value.x = 900;
                                       },
                                       [](std::uint32_t) {},
                                       [&](std::int16_t value) { texture = value; },
                                       [&](const sprite::Quad &value) { captured = value; },
                                       [] {}};
    auto view = sprite::screen_view(scratch.value);
    sprite::draw_screen_sprite(&view, 0, -1, 0x22, {1, 1, 1, 1}, services, fp);
    assert(texture == 7 && angles == 0 && pushes == 0 && captured.origin[0] == 10 &&
           captured.origin[1] == 20 && captured.horizontal[0] == 60 && captured.vertical[1] == 30);
    bool missing = false;
    try {
        sprite::draw_screen_sprite(&view, 0, -1, 1, {1, 1, 1, 1}, services, fp);
    } catch (const std::invalid_argument &) {
        missing = true;
    }
    assert(missing && pushes == 1 && angles == 0);
}
