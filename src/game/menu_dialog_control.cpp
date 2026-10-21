#include "game/menu_dialog_control.hpp"
namespace sadx::menu_dialog_control {
namespace {
// NOTSA: validates the canonical task/work at the native boundary without adding an owner.
std::shared_ptr<menu_dialog::Work> current_work(menu_dialog::Owner owner) {
    if (!owner.task)
        throw menu_dialog::Error(menu_dialog::Boundary::MissingTask,
                                 "dialog control has no canonical task");
    auto work = std::static_pointer_cast<menu_dialog::Work>(owner.task->auxiliary_native_work);
    if (!work)
        throw menu_dialog::Error(menu_dialog::Boundary::MissingWork,
                                 "dialog control has no canonical auxiliary work");
    return work;
}
} // namespace
bool close(menu_dialog::Owner owner) {
    const auto work = current_work(owner);
    if (work->phase < 2)
        return false;
    if (work->phase == 2) {
        work->opening_frames = 4;
        work->phase = 3;
    }
    return true;
}
std::int16_t idle_frames(menu_dialog::Owner owner) { return current_work(owner)->repeat_frames; }
void reset_idle_frames(std::int16_t frames, menu_dialog::Owner owner) {
    current_work(owner)->repeat_frames = frames;
}
void open_quick(const menu_dialog::Parameters &parameters, std::int8_t cursor,
                menu_dialog::Choices disabled, menu_dialog::Owner owner) {
    const auto work = current_work(owner);
    work->parameters = &parameters;
    if (disabled.data) {
        for (std::size_t index = 0;; ++index) {
            if (index >= disabled.size)
                throw menu_dialog::Error(menu_dialog::Boundary::DisabledTerminator,
                                         "quick dialog choices end before their terminator");
            if (index >= work->choice_ids.size())
                throw menu_dialog::Error(menu_dialog::Boundary::DisabledCapacity,
                                         "quick dialog choices exceed recovered storage");
            work->choice_ids[index] = disabled.data[index];
            // The source rereads the input after its store, preserving overlapping resources.
            if (disabled.data[index] < 0)
                break;
        }
    } else {
        work->choice_ids.fill(-1);
    }
    work->previous_choice = -1;
    menu_dialog::set_cursor(cursor, owner);
    work->repeat_frames = 0;
    work->animation_angle = 0;
    work->phase = 2;
}
void open_advanced(std::int32_t dialog, std::int8_t cursor, menu_dialog::Choices disabled,
                   Definitions &definitions, menu_dialog::Owner owner, menu_dialog::Calls &calls) {
    menu_dialog::open_with_cursor(definitions.parameters(dialog), cursor, disabled, owner, calls);
}
void open_advanced_quick(std::int32_t dialog, std::int8_t cursor, menu_dialog::Choices disabled,
                         Definitions &definitions, menu_dialog::Owner owner) {
    open_quick(definitions.parameters(dialog), cursor, disabled, owner);
}
} // namespace sadx::menu_dialog_control
