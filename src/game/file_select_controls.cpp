#include "game/file_select.hpp"
#include "cpu/fidelity_float.hpp"

namespace sadx::file_select {
std::uint32_t confirm_button_mask(std::uint32_t buttons) { return buttons & 12U; }
std::uint32_t cancel_button_mask(std::uint32_t buttons) { return buttons & 1026U; }
namespace {
// NOTSA: both source navigation branches converge on these live save stores.
void finish_selection(ControlState &state, ControlCalls &calls) {
    if (state.file_operation_pending != 0) state.file_operation_pending = 0;
    if (state.selected_file != state.displayed_save && state.displayed_save != 0) {
        calls.load_selected_save();
        calls.apply_save();
    }
    state.selected_file = state.displayed_save;
}
// NOTSA: the source two navigation directions wrap a16-bit word regardless of its signed view.
void step_first_visible(ControlState &state, std::uint16_t amount) {
    state.first_visible_file = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(state.first_visible_file) + amount);
}
} // namespace
void update_controls(title_menu::FileSelectWork &work, ControlState &state, ControlCalls &calls,
                     const NinjaMath &math, X87Context &fp) {
    // FileSelect initialization makes this shared animation counter known.
    auto angle = std::optional<std::uint32_t>(static_cast<std::uint32_t>(work.cursor_angle));
    title_menu::update_dialog_animation(work.cursor_alpha, angle, work.cursor_scale,
        state.logic_ticks, fp);
    work.cursor_angle = static_cast<std::int32_t>(*angle);
    work.cursor_color = 4294901760U;
    switch (work.submode) {
    case 0:
        work.submode = 1;
        return;
    case 1: {
        state.displayed_save = state.file_count == 0 ? 0 : state.selected_file;
        const auto pulse_angle = static_cast<std::int32_t>(
            state.logic_ticks *
            static_cast<std::uint32_t>(work.cursor_angle));
        const auto wave = fidelity::ExactFloat::retained(fp, math.cos_extended(pulse_angle));
        const auto factor = ((wave + 1.0f) * 0.5f).rounded();
        work.cursor_color = title_menu::interpolate_colors(4278190335U, 4294901760U, factor, fp);
        const auto pressed = static_cast<std::uint8_t>(calls.pressed_buttons());
        if ((pressed & 16U) != 0 && state.displayed_save != 0) {
            work.cursor_scale = 1.5f;
            --state.displayed_save;
            if (state.selected_card == 0) step_first_visible(state, 65535);
            else --state.selected_card;
            state.card_file = state.displayed_save;
            calls.play_navigation_sound();
        } else if ((pressed & 32U) != 0 && state.displayed_save < state.file_count) {
            work.cursor_scale = 1.5f;
            ++state.displayed_save;
            if (state.selected_card == 3) step_first_visible(state, 1);
            else ++state.selected_card;
            // MOVZX of the source byte is always less than65535.
            state.card_file = state.displayed_save;
            calls.play_navigation_sound();
        } else if (confirm_button_mask(calls.pressed_buttons()) != 0 && state.ready == 1) {
            if (state.displayed_save == 0) {
                if (state.file_count < 99) {
                    state.character = 0;
                    state.new_file_requested = 1;
                    calls.prepare_new_save();
                    calls.apply_save();
                    calls.request_save();
                    work.submode = 12;
                }
            } else {
                calls.request_dialog(0);
                work.submode = 2;
            }
        } else if (cancel_button_mask(calls.pressed_buttons()) != 0 && state.ready == 1 &&
                   work.presentation.previous != title_menu::MenuMode::OptionSelect) {
            if (state.alert_blocked != 0) {
                if (calls.close_warning() == 0) return;
                state.alert_blocked = 0;
            }
            work.submode = 9;
        }
        finish_selection(state, calls);
        return;
    }
    case 9:
        calls.transition_menu(work.presentation.previous);
        calls.play_cancel_sound();
        work.submode = 10;
        return;
    case 10:
        return;
    default:
        if (static_cast<std::uint32_t>(work.submode) > 17U) return;
        throw ControlStageError(work.submode);
    }
}
} // namespace sadx::file_select
