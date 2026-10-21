#include "engine/menu_square.hpp"
#include "cpu/fidelity_float.hpp"

namespace sadx::menu_square {
using fidelity::ExactFloat;
void draw(std::uint32_t color, float x, float y, float depth, float sx, float sy, View view,
          Calls &calls, X87Context &fp) {
    calls.select_square();
    calls.set_material(color | 16777215U);
    const auto offset_y = (ExactFloat(fp, y) - 240.0f).rounded();
    const auto offset_x = (ExactFloat(fp, x) - 320.0f).rounded();
    const auto shadow_y = (ExactFloat(fp, sy) * 1.177024483680725f).rounded();
    const auto shadow_x = (ExactFloat(fp, sx) * 1.0460431575775146f).rounded();
    const auto py = (ExactFloat(fp, view.vertical_scale) * 240.0f + offset_y).rounded();
    const auto px = (ExactFloat(fp, view.horizontal_scale) * 320.0f + offset_x).rounded();
    calls.draw_scaled(67108865U, px, py, depth, shadow_x, shadow_y);
    calls.set_material(color);
    const auto foreground_depth = (ExactFloat(fp, depth) - 0.3f).rounded();
    const auto foreground_y = (ExactFloat(fp, view.vertical_scale) * 240.0f + offset_y).rounded();
    const auto foreground_x = (ExactFloat(fp, view.horizontal_scale) * 320.0f + offset_x).rounded();
    calls.draw_scaled(67108864U, foreground_x, foreground_y, foreground_depth, sx, sy);
}
void draw_cursor(std::uint8_t alpha, float x, float y, float depth, float sx, float sy, View view,
                 Calls &calls, X87Context &fp) {
    calls.select_square();
    calls.set_material((static_cast<std::uint32_t>(alpha) << 24) | 15505408U);
    const auto py =
        ((ExactFloat(fp, y) - 240.0f) + ExactFloat(fp, view.vertical_scale) * 240.0f).rounded();
    const auto px =
        ((ExactFloat(fp, x) - 320.0f) + ExactFloat(fp, view.horizontal_scale) * 320.0f).rounded();
    calls.draw_scaled(67108866U, px, py, depth, sx, sy);
}
} // namespace sadx::menu_square
