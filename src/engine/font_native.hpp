#pragma once
#include "engine/native_texture_lifetime.hpp"
#include <array>
#include <cstdint>
#include <functional>

namespace sadx::font {
// GH_FONT_INFO; native resource identities replace the source's borrowed pointers.
struct Info {
    float char_width = 0, char_height = 0, tex_width = 0, tex_height = 0;
    texture_lifetime::List *textures = nullptr;
    const std::array<std::uint8_t, 128> *proportions = nullptr;
    std::array<std::uint32_t, 4> colors{};
};
struct State {
    float width = 0;
    float height = 0;
    const Info *source = nullptr;
    Info current{};
};
struct Calls {
    virtual ~Calls() = default;
    // NOTSA: composition binds the effective individual-texture loader.
    virtual void load_texture(texture_lifetime::List &) = 0;
};
// FUNCTION: SONIC 0x420a00 ABC_TXT_load
void load(const Info *, State &, Calls &);
// FUNCTION: SONIC 0x420a50 ABC_TXT_copy
void copy(const Info &, State &);
// FUNCTION: SONIC 0x420a30 ABC_TXT_release
void release(const Info &, const std::function<void(texture_lifetime::List &)> &);
} // namespace sadx::font
