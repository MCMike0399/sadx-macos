#include "runtime/native_session.hpp"
#include "runtime/native_title_scene.hpp"
namespace sadx::native {
void Session::capture_live_save() {
    // The mode representation conversion remains local to this synchronous, callback-free capture.
    const auto mode = static_cast<std::uint32_t>(flow_.mode);
    save::CaptureState state{mode, global_timer_, bootstrap_.character, bootstrap_.lives,
        soft_reset_.metal_sonic, flow_.playing_super_sonic, saved_stage_,
        bootstrap_.sequence_variables, bootstrap_.cutscene_flags,
        bootstrap_.section_work.data() + 12, bootstrap_.sequences, bootstrap_.completed_stages,
        missions_, rumble_enabled_, market_rings_};
    save::capture_live_data(save_, state);
}
void Session::write_live_save() {
    if (!save_writer_)
        throw flow::UnrecoveredGameFlowCall("native_save_writer_services");
    save::WriteState state{saving_disabled_, save_retry_, save_ok_, save_count_, flow_.save_phase,
        flow_.save_ready, save_exec_, flow_.alert_flag, file_delete_flag_, new_file_requested_,
        bootstrap_.card_operation, bootstrap_.menu.selected_file, bootstrap_.card_complete,
        save_name_, list_data_, save_};
    save::SaveNameCalls names{
        [this](const char *pointer) {
            if (pointer != selected_save_name_.get())
                throw std::runtime_error("native save filename has no matching allocation");
            selected_save_name_.reset();
        },
        [this](std::size_t bytes) {
            selected_save_name_.reset(new (std::nothrow) char[bytes]);
            return selected_save_name_.get();
        },
        [](char *destination, std::string_view value) {
            if (!destination) throw std::runtime_error("native save name formatter has no allocation");
            std::memcpy(destination, value.data(), value.size());
            destination[value.size()] = '\0';
        }};
    (void)save::write_file(state, *save_writer_, names, [this] { capture_live_save(); });
}
void Session::before_save_frame_completion() {
    if (!title_) throw flow::UnrecoveredGameFlowCall("native_save_message_services");
    title_->before_save_frame_completion();
}
void Session::after_save_frame_completion() {
    if (!title_) throw flow::UnrecoveredGameFlowCall("native_save_message_services");
    title_->after_save_frame_completion();
}
} // namespace sadx::native
