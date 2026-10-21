#pragma once
#include "game/file_select.hpp"
#include "game/save_file_list.hpp"
namespace sadx::menu_dialog {
struct ConfirmationState {
    file_select::ControlState &selection;
    std::uint16_t &selection_backup;
    std::uint8_t &fade_in_marker;
    const std::uint8_t &saved_character;
    std::uint8_t &character_cursor;
    const file_select::List &files;
    const char *&save_name;
};
struct ConfirmationCalls : file_select::ControlCalls {
    virtual std::int32_t dialog_result() = 0;
    // NOTSA: delete request uses the shared source dialog wrapper with its own resource definition.
    virtual void request_delete_confirmation() = 0;
    virtual void free_save_name(const char *) = 0;
    virtual char *allocate_save_name(std::size_t) = 0;
    virtual void copy_save_name(char *, std::string_view) = 0;
};
// FUNCTION: SONIC 0x5068e0 GetSavedCharacterCursor
std::uint8_t saved_character_cursor(std::uint8_t);
// FUNCTION: SONIC 0x505e90 RestoreSavedCharacterCursor
void restore_saved_character_cursor(const std::uint8_t &saved_character, std::uint8_t &cursor);
// NOTSA: the source confirmation stage is called after FileSelect's common animation prefix.
void confirm_selection(title_menu::FileSelectWork &, ConfirmationState &, ConfirmationCalls &);
} // namespace sadx::menu_dialog
