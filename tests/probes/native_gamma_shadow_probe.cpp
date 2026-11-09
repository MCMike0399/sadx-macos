#include "player/gamma_shadow.hpp"

#include <cstdint>
#include <cstring>

namespace {
enum Input : unsigned {
    X = 0, Y, Z, Yaw, Scale, Height, NormalZ, NormalX, Missed, ControlWord, Words
};
enum Output : unsigned {
    OutHeight = 0, OutQueryX, OutQueryY, OutQueryZ, OutTranslateX, OutTranslateY, OutTranslateZ,
    OutScaleX, OutScaleY, OutScaleZ, OutClip, OutRotZ, OutRotX, OutRotY, OutFlags, OutWords
};
enum Flag : std::uint32_t { Prepared = 1, Pushed = 2, Popped = 4, Drew = 8 };

float float_at(std::uint32_t word) {
    float value;
    std::memcpy(&value, &word, sizeof(value));
    return value;
}
std::uint32_t bits_of(float value) {
    std::uint32_t word;
    std::memcpy(&word, &value, sizeof(word));
    return word;
}

struct Adapter final : sadx::gamma_shadow::Services {
    const std::uint32_t *in;
    float query_x = 0, query_y = 0, query_z = 0;
    float tx = 0, ty = 0, tz = 0;
    float sx = 0, sy = 0, sz = 0, clip = 0;
    std::uint32_t rot_z = 0, rot_x = 0, rot_y = 0, flags = 0;
    explicit Adapter(const std::uint32_t *input) : in(input) {}
    sadx::gamma_shadow::Ground sample_ground(float x, float y, float z) override {
        query_x = x;
        query_y = y;
        query_z = z;
        return {float_at(in[Height]), static_cast<std::int32_t>(in[NormalZ]),
                static_cast<std::int32_t>(in[NormalX])};
    }
    bool missed_frames() override { return in[Missed] != 0; }
    void prepare() override { flags |= Prepared; }
    void push_matrix() override { flags |= Pushed; }
    void translate(float x, float y, float z) override { tx = x; ty = y; tz = z; }
    void rotate_z(std::uint16_t angle) override { rot_z = 0x10000u | angle; }
    void rotate_x(std::uint16_t angle) override { rot_x = 0x10000u | angle; }
    void rotate_y(std::uint16_t angle) override { rot_y = 0x10000u | angle; }
    void scale(float x, float y, float z) override { sx = x; sy = y; sz = z; }
    void draw(float value) override {
        clip = value;
        flags |= Drew;
    }
    void pop_matrix() override { flags |= Popped; }
};
} // namespace

extern "C" int sadx_native_gamma_shadow(const std::uint32_t *input, std::uint32_t *output) {
    try {
        Adapter services(input);
        sadx::gamma_shadow::Body body{float_at(input[X]), float_at(input[Y]), float_at(input[Z]),
                                      static_cast<std::int32_t>(input[Yaw])};
        sadx::fidelity::Context arithmetic(static_cast<std::uint16_t>(input[ControlWord]));
        const auto height = sadx::gamma_shadow::update(body, float_at(input[Scale]), services, arithmetic);
        std::memset(output, 0, OutWords * sizeof(std::uint32_t));
        output[OutHeight] = bits_of(height.rounded());
        output[OutQueryX] = bits_of(services.query_x);
        output[OutQueryY] = bits_of(services.query_y);
        output[OutQueryZ] = bits_of(services.query_z);
        output[OutTranslateX] = bits_of(services.tx);
        output[OutTranslateY] = bits_of(services.ty);
        output[OutTranslateZ] = bits_of(services.tz);
        output[OutScaleX] = bits_of(services.sx);
        output[OutScaleY] = bits_of(services.sy);
        output[OutScaleZ] = bits_of(services.sz);
        output[OutClip] = bits_of(services.clip);
        output[OutRotZ] = services.rot_z;
        output[OutRotX] = services.rot_x;
        output[OutRotY] = services.rot_y;
        output[OutFlags] = services.flags;
        return 1;
    } catch (...) {
        return 0;
    }
}
