#include "runtime/native_title_scene.hpp"
#include "runtime/native_session.hpp"
#include "runtime/native_display.hpp"
#include "engine/cooperative_thread_native.hpp"
#include <cstring>
#include <new>

namespace sadx::native {
void Session::attach_menu_dialogs(MenuDialogAssets assets) {
    if (menu_dialog_assets_)
        throw std::runtime_error("native menu dialogs are already attached");
    if (assets.definitions.size() != assets.panel_texture_bindings.size())
        throw std::runtime_error("native menu dialog texture bindings are incomplete");
    for (std::size_t index = 0; index < assets.definitions.size(); ++index) {
        const auto &binding = menu_texture_binding(assets.panel_texture_bindings[index]);
        if (!binding.textures)
            throw std::runtime_error("native menu dialog panel texture list is unavailable");
        assets.definitions[index].textures = binding.textures;
    }
    menu_dialog_assets_ = std::make_unique<MenuDialogAssets>(std::move(assets));
}
std::int32_t TitleScene::dialog_result() {
    return menu_dialog::result({state_.dialog_task});
}
void TitleScene::request_delete_confirmation() {
    throw flow::UnrecoveredGameFlowCall("native_file_select_delete_confirmation_5057d0");
}
void TitleScene::free_save_name(const char *name) {
    if (name != session_.selected_save_name_.get())
        throw std::runtime_error("native selected save name has no matching allocation");
    session_.selected_save_name_.reset();
}
char *TitleScene::allocate_save_name(std::size_t size) {
    session_.selected_save_name_.reset(new (std::nothrow) char[size]);
    return session_.selected_save_name_.get();
}
void TitleScene::copy_save_name(char *destination, std::string_view name) {
    if (!destination)
        throw flow::UnrecoveredGameFlowCall("native_selected_save_name_allocation");
    std::memcpy(destination, name.data(), name.size());
    destination[name.size()] = '\0';
}
void TitleScene::play_sound(std::int32_t cue) {
    sound_effects::Queue queue{session_.sound_, session_.bank_channels_, session_.sound_effects_};
    sound_effects::play_one_shot(cue, nullptr, 0, 0, queue);
}
void TitleScene::clear_screen_fade() { session_.logo_.set_fade_color(0); }
void TitleScene::set_back_colors(std::uint32_t top, std::uint32_t middle, std::uint32_t bottom) {
    display_.output().set_background(top, middle, bottom);
}
void TitleScene::select_cursors() {
    if (!session_.menu_dialog_assets_)
        throw flow::UnrecoveredGameFlowCall("native_menu_dialog_assets");
    const auto &binding = session_.menu_texture_binding(
        session_.menu_dialog_assets_->cursor_texture_binding);
    if (!binding.textures)
        throw std::runtime_error("native menu dialog cursor texture list is unavailable");
    display_.output().select_texture_list(*binding.textures);
}
void TitleScene::select_panels(texture_lifetime::List &list) {
    display_.output().select_texture_list(list);
}
void TitleScene::set_vertex_colors(const std::array<std::uint32_t, 4> &colors) {
    menu_draw::set_vertex_colors(colors, session_.logo_.drawing_state());
}
void TitleScene::draw_scaled(std::uint32_t texture, float x, float y, float depth,
                              float sx, float sy) {
    draw_scaled_texture(texture, x, y, depth, sx, sy);
}
void TitleScene::draw_shadow_window(float x, float y, float depth, float width, float height) {
    const auto frame = display_.frame_state();
    menu_draw::draw_shadow_window(x, y, depth, width, height, session_.logo_.drawing_state(),
                                  frame.display_disabled, *this, display_.arithmetic());
}
void TitleScene::draw_content(title_menu::DialogContent content,
                               const menu_dialog::ContentArguments &arguments) {
    if (content == title_menu::DialogContent::None) return;
    if (!session_.menu_dialog_assets_)
        throw flow::UnrecoveredGameFlowCall("native_menu_dialog_assets");
    menu_caption::draw_help(static_cast<std::int8_t>(arguments.cursor),
        session_.menu_dialog_assets_->help, session_.text_language_, session_.menu_caption_,
        session_.flow_.pause_state, *this);
}
std::shared_ptr<menu_caption::Buffer> TitleScene::allocate_buffer(std::size_t size) {
    try {
        auto buffer = std::make_shared<menu_caption::Buffer>(size);
        ++caption_resources_.allocated;
        return buffer;
    } catch (const std::bad_alloc &) {
        return {};
    }
}
void TitleScene::free_buffer(std::shared_ptr<menu_caption::Buffer> buffer) {
    // The caller clears its published handle after this source free boundary.
    if (buffer) {
        buffer->bytes.reset();
        ++caption_resources_.freed;
    }
}
void TitleScene::wait_for_cooperative_thread() {
    ++caption_resources_.waits;
    const auto result = cooperative_threads::wait(session_.logo_.current_cooperative_thread());
    if (result != cooperative_threads::WaitResult::Completed)
        throw flow::UnrecoveredGameFlowCall("native_cooperative_thread_continuation_4014bc");
}
void TitleScene::append_message(menu_caption::Context &, const std::string &) {
    throw flow::UnrecoveredGameFlowCall("native_message_puts_40e570");
}
void TitleScene::load_message_texture(menu_caption::Context &) {
    throw flow::UnrecoveredGameFlowCall("native_message_load_texture_40d290");
}
void TitleScene::draw_message(menu_caption::Context &) {
    throw flow::UnrecoveredGameFlowCall("native_message_display_40d490");
}
void TitleScene::release_texture_list(texture_lifetime::List &list) {
    display_.release_texture_list(list);
}
void TitleScene::release_queue_overflow() {
    throw std::runtime_error("native message texture release queue exceeds32 entries");
}
void TitleScene::complete_caption_frame() {
    auto &releases = display_.message_texture_releases();
    menu_caption::complete_frame(session_.menu_caption_, releases, *this);
    menu_caption::release_queued(releases, *this);
}
void Session::complete_menu_messages() {
    if (!title_)
        throw flow::UnrecoveredGameFlowCall("native_menu_message_services");
    title_->complete_caption_frame();
}
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
