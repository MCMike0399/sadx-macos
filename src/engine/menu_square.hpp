#pragma once
#include "cpu/x87_math.hpp"
#include <cstdint>

namespace sadx::menu_square {
// NOTSA: borrowed screen projection; the caller owns all display and renderer
// state.
struct View {
    const float &horizontal_scale, &vertical_scale;
};
struct Calls {
    virtual ~Calls() = default;
    virtual void select_square() = 0;
    virtual void set_material(std::uint32_t) = 0;
    virtual void draw_scaled(std::uint32_t, float x, float y, float depth, float sx, float sy) = 0;
};
// FUNCTION: SONIC 0x431cb0 DrawSquareC
void draw(std::uint32_t color, float x, float y, float depth, float sx, float sy, View, Calls &,
          X87Context &);
// FUNCTION: SONIC 0x431bb0 DrawSquareCursor
void draw_cursor(std::uint8_t alpha, float x, float y, float depth, float sx, float sy, View,
                 Calls &, X87Context &);
} // namespace sadx::menu_square
