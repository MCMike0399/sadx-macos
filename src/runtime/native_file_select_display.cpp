#include "runtime/native_title_scene.hpp"
#include "runtime/native_session.hpp"
#include "runtime/native_display.hpp"

namespace sadx::native {
void TitleScene::draw_file_select(title_menu::FileSelectWork &work) {
    if (!session_.file_select_display_assets_)
        throw flow::UnrecoveredGameFlowCall("native_file_select_display_assets");
    const auto frame = display_.frame_state();
    const auto &viewport = display_.viewport();
    file_select_display::View view{
        frame.horizontal_scale, frame.vertical_scale, frame.display_disabled,
        viewport.width, viewport.height, session_.save_, session_.bootstrap_.sequence_variables,
        session_.bootstrap_.menu.flags[0], session_.flow_.tutorial_variant,
        session_.displayed_save_index_, session_.first_visible_file_,
        session_.bootstrap_.card_operation, session_.selected_card_index_,
        session_.bootstrap_.card_state, session_.list_data_, session_.font_state_,
        session_.file_select_display_assets_->tables, session_.file_message_visible_,
        session_.flow_.alert_blocked, session_.file_message_, session_.save_warning_message_};
    file_select_display::draw(work, view, session_.logo_.drawing_state(), *this,
                              display_.arithmetic());
}
void TitleScene::select_menu_texture(file_select_display::MenuTexture texture) {
    const auto &binding = session_.menu_texture_binding(static_cast<std::uint32_t>(texture));
    if (!binding.textures)
        throw std::runtime_error("native file-select menu texture list is unavailable");
    display_.output().select_texture_list(*binding.textures);
}
void TitleScene::select_standard_texture(menu_draw::StandardTexture texture) {
    // Both source operands alias lists already loaded by the FileSelect menu catalog.
    const auto index = texture == menu_draw::StandardTexture::Window ? 1U : 10U;
    const auto &binding = session_.menu_texture_binding(index);
    if (!binding.textures)
        throw std::runtime_error("native standard menu texture list is unavailable");
    display_.output().select_texture_list(*binding.textures);
}
file_select_display::Calendar TitleScene::local_calendar(std::uint64_t filetime) {
    if (!session_.file_time_calendar_)
        throw flow::UnrecoveredGameFlowCall("native_file_time_local_calendar");
    return session_.file_time_calendar_(filetime);
}
void TitleScene::display_message(const TaskHandle &) {
    throw flow::UnrecoveredGameFlowCall("native_file_select_message_display");
}
void TitleScene::set_blend(std::uint32_t target, std::uint32_t mode) {
    display_.output().set_blending(target, mode);
}
void TitleScene::reset_material() { display_.reset_material(); }
void TitleScene::set_depth_compare(std::uint32_t mode) { display_.set_depth_comparison(mode); }
void TitleScene::set_depth_write(std::uint32_t enabled) { display_.set_depth_write(enabled != 0); }
void TitleScene::restore_depth_compare() { display_.restore_depth_comparison(); }
pvr::TextureInfo TitleScene::texture(std::uint32_t index) { return display_.output().texture(index); }
void TitleScene::draw_texture(const std::array<pvr::Vertex, 4> &vertices,
                              std::uint32_t global_index, std::uint32_t alpha) {
    display_.output().draw_texture(vertices, global_index, alpha);
}
void TitleScene::draw_polygon(const std::array<pvr::Point, 4> &points, std::uint32_t color,
                              float depth, std::uint32_t flags) {
    display_.output().draw_polygon(points, color, depth, flags);
}
void TitleScene::draw_colored_polygon(const std::array<pvr::Point, 4> &points,
                                      const std::array<std::uint32_t, 4> &colors,
                                      float depth, std::uint32_t flags) {
    display_.output().draw_colored_polygon(points, colors, depth, flags);
}
std::uint32_t TitleScene::texture_global_index(const texture_lifetime::List &list,
                                              std::uint32_t index) {
    return display_.output().texture_global_index(list, index);
}
void TitleScene::queue_box(float left, float top, float right, float bottom, float depth,
                           std::uint32_t color) {
    display_.output().queue_box(left, top, right, bottom, depth, color);
}
} // namespace sadx::native
