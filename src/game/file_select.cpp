#include "game/file_select.hpp"
#include "cpu/fidelity_float.hpp"

namespace sadx::file_select {
void initialize(title_menu::FileSelectWork &work, InitState &shared, InitCalls &calls) {
    calls.load_mode_textures(title_menu::MenuMode::FileSelect);
    calls.request_menu_music(2);
    calls.request_menu_voice(1);
    work.submode = 0;
    shared.selected_file = shared.file_count == 0 ? 0 : shared.card_file;
    work.selected_character = 0;
    work.cursor_angle = 0;
    work.cursor_color = 4294901760U;
    work.player_backup = static_cast<std::int32_t>(shared.save_word_before_selection);
    shared.file_count = calls.count_save_files();
    shared.menu_counter = 0;
    shared.saving_disabled = 0;
    if (shared.file_count != 0) {
        shared.card_file = 1;
        shared.list_has_files = 1;
        shared.selection_has_files = 1;
        calls.load_selected_save();
        shared.selected_file = 1;
        shared.card_file = 1;
    } else {
        shared.card_file = 0;
        shared.list_has_files = 0;
        shared.selection_has_files = 0;
    }
    calls.load_font();
    calls.copy_font();
    calls.apply_save();
}
bool check_save(const save::EncodedData &bytes) {
    const auto stored = static_cast<std::uint16_t>(bytes[0] | (static_cast<std::uint16_t>(bytes[1]) << 8));
    return save::checksum(bytes) == stored;
}
void update(title_menu::FileSelectWork &work, title_menu::State &menus,
            const std::uint32_t &logic_ticks, std::uint8_t &fade_in_marker,
            UpdateCalls &calls, X87Context &fp) {
    using title_menu::MenuMode;
    using title_menu::TitlePhase;
    if (menus.sequence_work->mode == MenuMode::FileSelect &&
        work.presentation.phase == TitlePhase::RequestWait) {
        calls.initialize_file_select(work);
        work.presentation.fade_progress = 0;
        work.presentation.phase = TitlePhase::FadeIn;
    }
    if (menus.sequence_work->old_mode == MenuMode::FileSelect &&
        work.presentation.phase == TitlePhase::Keep) {
        work.presentation.phase = TitlePhase::FadeOut;
        work.presentation.fade_progress = 0;
    }
    switch (work.presentation.phase) {
    case TitlePhase::RequestWait:
        work.presentation.color = 0;
        break;
    case TitlePhase::FadeIn: {
        work.presentation.color = calls.fade_out_color(work.presentation.fade_progress);
        const auto increment =
            fidelity::ExactFloat::integer(fp, static_cast<std::int32_t>(logic_ticks)).multiply_double(0.05);
        const auto progress = increment + work.presentation.fade_progress;
        work.presentation.fade_progress = progress.rounded();
        const auto comparison = progress.compare(1.0f);
        if (comparison == X87Comparison::equal || comparison == X87Comparison::greater)
            work.presentation.phase = TitlePhase::Keep;
        fade_in_marker = 1;
        break;
    }
    case TitlePhase::Keep:
        work.presentation.color = 4294967295U;
        menus.sequence_work->fixed_mode = MenuMode::FileSelect;
        calls.update_controls(work);
        break;
    case TitlePhase::FadeOut: {
        work.presentation.color = calls.fade_in_color(work.presentation.fade_progress);
        const auto increment =
            fidelity::ExactFloat::integer(fp, static_cast<std::int32_t>(logic_ticks)).multiply_double(0.05);
        const auto progress = increment + work.presentation.fade_progress;
        work.presentation.fade_progress = progress.rounded();
        if (progress.compare(1.0f) == X87Comparison::greater)
            work.presentation.fade_progress = 1.0f;
        const auto comparison = fidelity::ExactFloat(fp, work.presentation.fade_progress).compare(1.0f);
        if (comparison == X87Comparison::equal || comparison == X87Comparison::greater) {
            calls.release_mode_textures(MenuMode::FileSelect);
            calls.release_font();
            menus.sequence_work->fade_end = 1;
            work.presentation.phase = TitlePhase::RequestWait;
            calls.free_save_list();
        }
        break;
    }
    default:
        break;
    }
}
} // namespace sadx::file_select
