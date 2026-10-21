#include "game/menu_dialog_file_select.hpp"
#include "game/game_bootstrap.hpp"
namespace sadx::menu_dialog {
std::uint8_t saved_character_cursor(std::uint8_t character) {
    switch (character) {
    case 0:
        return 1;
    case 2:
        return 2;
    case 3:
        return 3;
    case 5:
        return 4;
    case 6:
        return 5;
    case 7:
        return 6;
    case 8:
        return 7;
    case 9:
        return 8;
    default:
        return 0;
    }
}
void restore_saved_character_cursor(const std::uint8_t &saved_character, std::uint8_t &cursor) {
    (void)bootstrap::loading_status();
    (void)bootstrap::loading_status();
    cursor = saved_character_cursor(saved_character);
}
namespace {
// NOTSA: source stage2 success traverses the live list only after the transition callbacks.
void publish_selected_name(ConfirmationState &state, ConfirmationCalls &calls) {
    auto node = state.files;
    for (unsigned remaining = state.selection.card_file; remaining != 0; --remaining) {
        if (!node)
            throw file_select::Error(file_select::Boundary::ListIndex,
                                     "selected save node is absent");
        node = node->next;
    }
    if (!node)
        throw file_select::Error(file_select::Boundary::ListIndex, "selected save node is absent");
    if (state.save_name) {
        calls.free_save_name(state.save_name);
        state.save_name = nullptr;
    }
    if (!node->file)
        throw file_select::Error(file_select::Boundary::IndeterminateSentinel,
                                 "selected save filename is indeterminate");
    const auto &name = node->file->name;
    auto *copy = calls.allocate_save_name(name.size() + 1);
    state.save_name = copy;
    calls.copy_save_name(copy, name);
}
} // namespace
// NOTSA: named stage helper; FileSelect_Controls owns the common animation and dispatch.
void confirm_selection(title_menu::FileSelectWork &work, ConfirmationState &state,
                       ConfirmationCalls &calls) {
    const auto choice = static_cast<std::int8_t>(calls.dialog_result());
    if (choice < 0)
        return;
    switch (choice) {
    case 0:
        state.selection.card_file = state.selection.displayed_save;
        (void)bootstrap::loading_status();
        work.wait_frames = 0;
        if (file_select::control_stage(work) == file_select::ControlStage::ConfirmExisting) {
            calls.transition_menu(work.presentation.next);
            state.fade_in_marker = 0;
            state.selection_backup = state.selection.card_file;
            restore_saved_character_cursor(state.saved_character, state.character_cursor);
            file_select::set_control_stage(work, file_select::ControlStage::Leaving);
            publish_selected_name(state, calls);
        }
        return;
    case 1:
        file_select::set_control_stage(work, file_select::ControlStage::Initialize);
        state.selection.selected_file =
            state.selection.file_count == 0 ? 0 : state.selection.card_file;
        work.selected_character = 0;
        work.cursor_angle = 0;
        work.cursor_color = 4294901760U;
        return;
    case 2:
        calls.request_delete_confirmation();
        file_select::set_control_stage(work, file_select::ControlStage::ConfirmDelete);
        return;
    default:
        return;
    }
}
} // namespace sadx::menu_dialog
