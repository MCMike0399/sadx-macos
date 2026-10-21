#include "engine/screen_sprite.hpp"
#include <cstring>
extern "C" int sadx_native_center_screen_sprite(const unsigned *v, unsigned *out) {
    using namespace sadx;
    auto f = [](unsigned b) {
        float x;
        std::memcpy(&x, &b, 4);
        return x;
    };
    sprite::ScreenSprite source;
    source.x = f(v[1]);
    source.y = f(v[2]);
    source.scale_x = f(v[4]);
    source.scale_y = f(v[5]);
    source.copied_z_bits = v[3];
    source.angle = static_cast<int>(v[6]);
    sprite::ScreenSpriteScratch scratch;
    X87Context fp(static_cast<unsigned short>(v[0]));
    float h = f(v[9]), vertical = f(v[10]);
    sprite::center_screen_sprite(scratch, source, h, vertical, fp);
    std::memcpy(out, &scratch.value.x, 4);
    std::memcpy(out + 1, &scratch.value.y, 4);
    out[2] = *scratch.value.copied_z_bits;
    std::memcpy(out + 3, &scratch.value.scale_x, 4);
    std::memcpy(out + 4, &scratch.value.scale_y, 4);
    out[5] = static_cast<unsigned>(*scratch.value.angle);
    out[6] = v[7];
    out[7] = v[8];
    out[8] = fp.exception_flags() & 63;
    return 1;
}
