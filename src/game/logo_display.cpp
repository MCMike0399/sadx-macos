#include "game/logo.hpp"
#include "cpu/crt_integer.hpp"
#include "cpu/fidelity_float.hpp"
#include <stdexcept>

namespace sadx::logo {
void display(std::uint8_t first_texture, DisplayState &state, DisplayCalls &calls,
             X87Context &fp) {
    calls.select_logo_textures();
    calls.set_material(4294967295U);
    calls.default_blending();
    using fidelity::ExactFloat;
    const auto exact = [&](float value) {
        return ExactFloat(fp, value);
    };
    auto alpha = ExactFloat::integer(fp, state.frames);
    if (alpha.compare(32.0f) == X87Comparison::less)
        alpha = alpha * 7.96875f;
    else
        alpha = exact(255.0f);
    const auto color = ((static_cast<std::uint32_t>(alpha.truncated_integer()) & 255U) << 24) |
                       16777215U;
    calls.set_base_color(color);
    std::uint32_t texture = first_texture;
    for (std::int32_t row = 0; row < 2; ++row) {
        const auto row_offset =
            ((ExactFloat::integer(fp, row) * 256.0f - 16.0f) - 240.0f).rounded();
        for (std::int32_t column = 0; column < 3; ++column) {
            const auto y = (exact(state.vertical_scale) * 240.0f + row_offset).rounded();
            const auto x_offset =
                ExactFloat::integer(fp, column) * 256.0f - 64.0f - 320.0f;
            const auto x_center = exact(state.horizontal_scale) * 320.0f;
            const auto x = (x_offset + x_center).rounded();
            calls.draw_texture(texture++, x, y, 1.1f);
        }
    }
    const auto background_color = state.first_vertex_color;
    const auto height = (exact(state.vertical_scale) * 480.0f).rounded();
    const auto width = (exact(state.horizontal_scale) * 640.0f).rounded();
    calls.draw_box(width, height, -11.0f, background_color);
}
void display_profile_640(std::uint8_t first_texture, DisplayState &state, DisplayCalls &calls,
                         X87Context &fp, std::uint32_t width, std::uint32_t height) {
    // Loader bgscale.cpp:38 -> scale_function(Align_Center, true): scale_push uses
    // unit stretch. uiscale.cpp update_parameters/get_offset/get_scale give scale=1,
    // offset=(0,0) at 640x480. This adapter deliberately admits only that identity.
    if (width != 640 || height != 480 || state.horizontal_scale != 1.0f ||
        state.vertical_scale != 1.0f)
        throw std::runtime_error("uncomposed logo background scaling outside 640x480");
    display(first_texture, state, calls, fp);
}
} // namespace sadx::logo
