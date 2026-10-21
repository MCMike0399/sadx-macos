#include "game/save_completion.hpp"
namespace sadx::file_select {
namespace {
// NOTSA: shared source stores before closing the failed-save warning.
void clear_pending(SaveCompletionState &state) {
    state.now_saving = 0;
    state.save_ready = 0;
    state.save_exec_flag = 0;
    state.alert_flag = 0;
}
} // namespace
bool update_save_completion(title_menu::FileSelectWork &work, SaveCompletionState &state,
                            SaveCompletionCalls &calls) {
    if (work.submode != 12 && work.submode != 13)
        return false;
    if (work.submode == 12) {
        if (work.wait_frames < 5) {
            state.alert_flag = 0;
            ++work.wait_frames;
            return true;
        }
        if (state.alert_blocked == 0) {
            work.wait_frames = 0;
            state.fade_in_marker = 0;
            if (static_cast<std::int8_t>(state.save_ok) < 1) {
                state.alert_blocked = 1;
                clear_pending(state);
                calls.request_dialog(17);
                state.warning_task = calls.create_save_warning();
            }
        }
        work.submode = 13;
    }
    if (work.wait_frames < 5) {
        ++work.wait_frames;
        return true;
    }
    if (static_cast<std::int8_t>(state.save_ok) > 0) {
        state.alert_blocked = 0;
        calls.transition_menu(work.presentation.next);
        state.selected_file_backup = state.selected_card_file;
        state.fade_in_marker = 0;
        work.submode = 10;
        state.save_ok = 0;
        return true;
    }
    const auto selection = calls.dialog_selection();
    if (selection >= 0) {
        clear_pending(state);
        state.fade_in_marker = 1;
        state.save_ok = 0;
        state.alert_blocked = 0;
        calls.close_save_warning(state.warning_task);
        if (selection == 0)
            work.submode = 0;
        else {
            calls.transition_menu(work.presentation.next);
            work.submode = 10;
            state.fade_in_marker = 0;
            state.saving_disabled = 1;
            calls.prepare_new_save();
            calls.apply_save();
        }
    }
    return true;
}
} // namespace sadx::file_select
