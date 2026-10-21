#include "cpu/fidelity_float.hpp"
#include "game/menu_dialog.hpp"
namespace sadx::menu_dialog {
namespace {
using fidelity::ExactFloat;
// NOTSA: repeated center-relative source projection, retaining the x87 intermediate.
float centered(float coordinate, float scale, float center, X87Context &fp) {
    return (ExactFloat(fp, scale) * center + (ExactFloat(fp, coordinate) - center)).rounded();
}
// NOTSA: bounds the active source parameter dereference with an explicit resource failure.
const Parameters &parameters_of(const Work &work) {
    if (!work.parameters)
        throw Error(Boundary::MissingParameters, "drawn dialog has no parameters");
    return *work.parameters;
}
// NOTSA: bounds the imported panel array without inventing absent source rows.
const title_menu::DialogPanel &panel_of(const Parameters &parameters, int choice) {
    if (!parameters.panels || choice < 0 ||
        static_cast<std::size_t>(choice) >= parameters.panels->size())
        throw Error(Boundary::DialogIndex, "drawn dialog choice has no imported panel");
    return (*parameters.panels)[static_cast<std::size_t>(choice)];
}
// NOTSA: repeated source binary32 depth subtraction.
float offset_depth(float depth, float offset, X87Context &fp) {
    return (ExactFloat(fp, depth) - offset).rounded();
}
} // namespace
void draw_round(std::uint32_t color, float x, float y, float depth, float sx, float sy,
                DrawCalls &calls, X87Context &fp) {
    calls.select_cursors();
    calls.set_material(color);
    calls.draw_scaled(67108866U, x, y, offset_depth(depth, 0.3f, fp), sx, sy);
    calls.set_material(color | 16777215U);
    calls.draw_scaled(67108865U, x, y, offset_depth(depth, 0.6f, fp), sx, sy);
}
void draw_round_cursor(std::uint8_t alpha, float x, float y, float depth, float sx, float sy,
                       DrawState state, DrawCalls &calls, X87Context &fp) {
    const auto lower = (static_cast<std::uint32_t>(alpha) << 24U) | 15505408U;
    const auto upper = (static_cast<std::uint32_t>(alpha) << 24U) | 16771840U;
    calls.select_cursors();
    calls.set_vertex_colors({upper, lower, upper, lower});
    const auto px = centered(x, state.screen_scale_x, 320.0f, fp);
    const auto py = centered(y, state.screen_scale_y, 240.0f, fp);
    const auto width = (ExactFloat(fp, sx) * 0.25f).rounded();
    const auto height = (ExactFloat(fp, sy) * 0.25f).rounded();
    calls.draw_scaled(67108867U, px, py, depth, width, height);
}
void draw_window_box(float x, float y, float depth, float width, float height, DrawState state,
                     DrawCalls &calls, X87Context &fp) {
    const auto left = (ExactFloat(fp, state.screen_scale_x) * 320.0f +
                       ((ExactFloat(fp, x) - ExactFloat(fp, width) * 0.5f) - 320.0f))
                          .rounded();
    const auto top = (ExactFloat(fp, state.screen_scale_y) * 240.0f +
                      ((ExactFloat(fp, y) - ExactFloat(fp, height) * 0.5f) - 240.0f))
                         .rounded();
    calls.draw_shadow_window(left, top, depth, width, height);
}
void draw_window(const Work &work, DrawState state, DrawCalls &calls, X87Context &fp) {
    const auto &parameters = parameters_of(work);
    calls.default_blending();
    calls.set_vertex_colors({parameters.upper_color, parameters.lower_color, parameters.upper_color,
                             parameters.lower_color});
    const auto depth = offset_depth(parameters.depth, 4.0f, fp);
    const auto width = (ExactFloat(fp, parameters.width) * work.scale_x).rounded();
    const auto height = (ExactFloat(fp, parameters.height) * work.scale_y).rounded();
    draw_window_box(parameters.center_x, parameters.center_y, depth, width, height, state, calls,
                    fp);
}
void draw_panels(const Work &work, Owner owner, DrawState state, DrawCalls &calls, X87Context &fp) {
    const auto &parameters = parameters_of(work);
    if (parameters.style != title_menu::DialogStyle::BlueFourth)
        throw Error(Boundary::NavigationStyle,
                    "dialog panel layout is not recovered for this style");
    for (int choice = 0; choice < parameters.choice_count; ++choice) {
        const auto &panel = panel_of(parameters, choice);
        const auto x = (ExactFloat(fp, panel.x) * work.scale_x + parameters.center_x).rounded();
        const auto y = (ExactFloat(fp, panel.y) * work.scale_y + parameters.center_y).rounded();
        const auto disabled = work.previous_choice == choice ||
                              is_disabled(static_cast<std::int8_t>(choice), work, owner);
        const auto color = disabled ? (work.lower_color & 16777215U) | 805306368U
                           : static_cast<std::int8_t>(work.cursor) == choice ? work.upper_color
                                                                             : work.lower_color;
        const auto px = centered(x, state.screen_scale_x, 320.0f, fp);
        const auto py = centered(y, state.screen_scale_y, 240.0f, fp);
        const auto sx = (ExactFloat(fp, work.scale_x) * parameters.scale_x).rounded();
        const auto sy = (ExactFloat(fp, work.scale_y) * parameters.scale_y).rounded();
        draw_round(color, px, py, offset_depth(parameters.depth, 6.0f, fp), sx, sy, calls, fp);
        if (!parameters.textures)
            throw Error(Boundary::MissingParameters, "dialog panel texture list is absent");
        calls.select_panels(*parameters.textures);
        calls.set_material(disabled ? 2969567231U : 4294967295U);
        calls.draw_scaled(static_cast<std::uint32_t>(panel.texture) | 67108864U, px, py,
                          offset_depth(parameters.depth, 8.0f, fp), work.scale_x, work.scale_y);
    }
    const auto cursor = static_cast<std::int8_t>(work.cursor);
    if (cursor >= parameters.choice_count)
        return;
    const auto &panel = panel_of(parameters, cursor);
    auto alpha = work.animation_value;
    if (is_disabled(cursor, work, owner))
        alpha >>= 1U;
    const auto sx = (ExactFloat(fp, parameters.scale_x) * work.fade * work.scale_x).rounded();
    const auto sy = (ExactFloat(fp, parameters.scale_y) * work.scale_y * work.fade).rounded();
    const auto x = (ExactFloat(fp, panel.x) * work.scale_x + parameters.center_x).rounded();
    const auto y = (ExactFloat(fp, panel.y) * work.scale_y + parameters.center_y).rounded();
    draw_round_cursor(alpha, x, y, offset_depth(parameters.depth, 10.0f, fp), sx, sy, state, calls,
                      fp);
}
void draw(const Work &work, Owner owner, DrawState state, DrawCalls &calls, X87Context &fp) {
    if (title_menu::draw_dialog(work, fp) == title_menu::MenuDrawResult::Complete)
        return;
    calls.set_base_color(work.color);
    if (state.window_after_content == 0)
        draw_window(work, state, calls, fp);
    const auto &parameters = parameters_of(work);
    if (parameters.content != title_menu::DialogContent::None)
        calls.draw_content(parameters.content,
                           {parameters.center_x, parameters.center_y, parameters.depth,
                            work.scale_x, work.scale_y, work.cursor});
    if (state.window_after_content != 0)
        draw_window(work, state, calls, fp);
    draw_panels(work, owner, state, calls, fp);
    state.window_after_content = 0;
}
} // namespace sadx::menu_dialog
