#include "game/saving_message.hpp"
#include "cpu/fidelity_float.hpp"
namespace sadx::saving_message {
void display(StateView state, Calls &calls, X87Context &fp) {
    if (state.pending_message == 0) {
        const auto bitmap = calls.bitmap_from_arena_end(131104);
        menu_caption::open_message(state.message, 460, 40, 140, 80, 3489660968U, bitmap,
                                   state.pause_state, calls);
        state.message.spacing_x = 0;
        state.message.spacing_y = 0;
        state.message.background = 0;
        menu_caption::clear_message(state.message, state.pause_state, calls);
        message_text::put_text(state.message, state.text, state.fonts, state.language);
        message_text::request_texture_upload(state.message);
        state.pending_message = 1;
    }
    if (state.pause_state == 1) {
        calls.disable_fog();
        const auto bottom = (fidelity::ExactFloat(fp, state.vertical_scale) * 480.0f).rounded();
        const auto right = (fidelity::ExactFloat(fp, state.horizontal_scale) * 640.0f).rounded();
        calls.draw_black_box(right, bottom, -1.0f, 4278190080U);
        state.pause_state = 3;
    }
    if (state.reset_pending == 0) {
        state.reset_pending = 1;
        calls.reset_draw_queue();
        calls.skip_queue_execution();
    }
    calls.draw_message(state.message);
}
} // namespace sadx::saving_message
