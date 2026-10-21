#pragma once
#include "game/menu_dialog.hpp"

namespace sadx::menu_dialog_control {
// NOTSA: sparse imported source definitions; resource ids never require fabricated gap entries.
struct Definitions {
    virtual ~Definitions() = default;
    virtual const menu_dialog::Parameters &parameters(std::int32_t dialog) = 0;
};
// FUNCTION: SONIC 0x432580 CloseDialog
bool close(menu_dialog::Owner);
// FUNCTION: SONIC 0x4325b0 GetDialogIdleFrames
std::int16_t idle_frames(menu_dialog::Owner);
// FUNCTION: SONIC 0x4325c0 SetDialogIdleFrames
void reset_idle_frames(std::int16_t, menu_dialog::Owner);
// FUNCTION: SONIC 0x432cb0 OpenDialogCsrLetQuick
void open_quick(const menu_dialog::Parameters &, std::int8_t, menu_dialog::Choices,
                menu_dialog::Owner);
// FUNCTION: SONIC 0x5057d0 AdvaOpenDialogCsrLet
void open_advanced(std::int32_t, std::int8_t, menu_dialog::Choices, Definitions &,
                   menu_dialog::Owner, menu_dialog::Calls &);
// FUNCTION: SONIC 0x5057f0 AdvaOpenDialogCsrLetQuick
void open_advanced_quick(std::int32_t, std::int8_t, menu_dialog::Choices, Definitions &,
                         menu_dialog::Owner);
} // namespace sadx::menu_dialog_control
