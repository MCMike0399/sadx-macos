#include "engine/ninja_view_native.hpp"
#include <cstring>
extern "C" int sadx_native_fov(const float *table, const float *, std::int32_t angle,
                               std::uint32_t width_bits, std::uint16_t cw, std::uint32_t *out) {
    sadx::NinjaMath::SineTable sine;
    std::memcpy(sine.data(), table, sizeof(sine));
    float width;
    std::memcpy(&width, &width_bits, sizeof(width));
    sadx::view::State state;
    sadx::X87Context fp(cw);
    sadx::view::set_screen_distance(angle, width, state, sine, fp);
    out[0] = 1;
    out[1] = 0;
    std::memcpy(out + 2, &state.sprite_scale, sizeof(state.sprite_scale));
    out[3] = fp.exception_flags() & 63;
    return 1;
}
