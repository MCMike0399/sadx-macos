#pragma once
#include "engine/task.hpp"
#include "game/title_menu.hpp"
#include <cstdint>
namespace sadx::file_select {
struct SaveCompletionState {
    std::uint8_t &alert_blocked;
    std::uint8_t &save_ok;
    std::uint8_t &fade_in_marker;
    std::int32_t &now_saving;
    std::uint8_t &save_ready;
    std::uint8_t &save_exec_flag;
    std::uint8_t &alert_flag;
    const std::uint8_t &selected_card_file;
    std::uint16_t &selected_file_backup;
    std::int32_t &saving_disabled;
    TaskHandle &warning_task;
};
struct SaveCompletionCalls {
    virtual ~SaveCompletionCalls() = default;
    virtual void request_dialog(std::int32_t index) = 0;
    // Source421ab0(7, language-specific save-warning string).
    virtual TaskHandle create_save_warning() = 0;
    virtual std::int8_t dialog_selection() = 0;
    virtual void close_save_warning(TaskHandle) = 0;
    virtual void transition_menu(title_menu::MenuMode) = 0;
    virtual void prepare_new_save() = 0;
    virtual void apply_save() = 0;
};
// NOTSA:503600 stages12/13 delegated by its existing readable dispatcher AFTER
// the shared animation prefix. Returns false only when a different stage owns
// work.
bool update_save_completion(title_menu::FileSelectWork &, SaveCompletionState &,
                            SaveCompletionCalls &);
} // namespace sadx::file_select
