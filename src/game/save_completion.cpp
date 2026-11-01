#include "game/save_completion.hpp"
#include "game/file_select.hpp"
namespace sadx::file_select {
namespace {
constexpr std::int16_t kSaveCompletionDelay = 5;
constexpr std::int32_t kSaveFailureDialog = 17;
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
    if (control_stage(work) != ControlStage::WaitForNewSave &&
        control_stage(work) != ControlStage::CompleteNewSave)
        return false;
    if (control_stage(work) == ControlStage::WaitForNewSave) {
        if (work.wait_frames < kSaveCompletionDelay) {
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
                calls.request_dialog(kSaveFailureDialog);
                state.warning_task = calls.create_save_warning();
            }
        }
        set_control_stage(work, ControlStage::CompleteNewSave);
        // Source falls through and consumes the first completion wait frame now.
    }
    if (work.wait_frames < kSaveCompletionDelay) {
        ++work.wait_frames;
        return true;
    }
    if (static_cast<std::int8_t>(state.save_ok) > 0) {
        state.alert_blocked = 0;
        // Transition callbacks can change the selected file; read its owner afterwards.
        calls.transition_menu(work.presentation.next);
        state.selected_file_backup = state.selected_card_file;
        state.fade_in_marker = 0;
        set_control_stage(work, ControlStage::Leaving);
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
            set_control_stage(work, ControlStage::Initialize);
        else {
            calls.transition_menu(work.presentation.next);
            set_control_stage(work, ControlStage::Leaving);
            state.fade_in_marker = 0;
            state.saving_disabled = 1;
            calls.prepare_new_save();
            calls.apply_save();
        }
    }
    return true;
}
} // namespace sadx::file_select
