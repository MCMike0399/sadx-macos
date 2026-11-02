#include "game/selection_preview.hpp"

namespace sadx::selection_preview {
void select_motion(State &state, std::int32_t motion) {
    // Keep the current owner alive while borrowing its work, as for draw/update.
    const auto task = state.task;
    if (!task)
        throw Error(Boundary::Task, 0, "motion selection has no preview task");
    const auto work = std::static_pointer_cast<Work>(task->auxiliary_native_work);
    if (!work)
        throw Error(Boundary::Work, 0, "motion selection has no preview work");
    work->selected_motion = static_cast<std::uint8_t>(motion);
}

void update_actions(Work &work, ActionState &state, const std::uint8_t &selected, ActionCalls &calls) {
    constexpr std::int8_t kEventInput = 12;
    constexpr std::int8_t kMilesPreviewInput = 49;
    constexpr std::uint32_t kMilesCounterFlag = 0x20000000;
    for (std::size_t slot = 0; slot < work.actors.size(); ++slot) {
        const auto task = work.actors[slot];
        if (!task)
            continue;
        const auto primary = calls.primary(task);
        if (slot == 6)
            continue;
        calls.set_input(static_cast<std::uint8_t>(slot), kEventInput);
        if (slot == 1) {
            auto &counter = calls.miles_counter();
            counter = static_cast<std::int32_t>(static_cast<std::uint32_t>(counter) &
                                                ~kMilesCounterFlag);
            calls.set_input(static_cast<std::uint8_t>(slot), kMilesPreviewInput);
        }
        const float speed = slot == 1 ? 0.5f : 0.25f;
        if (work.motion_counts[slot] == 0) {
            const auto action = calls.action(slot, 0);
            auto &textures = calls.textures(slot);
            calls.set_action(task, action, textures, speed, true, 0);
            work.motion_counts[slot] = 1;
        } else if (work.motion_counts[slot] == 1 && work.selected_motion != 0 && selected == slot) {
            if (calls.action(slot, 1).identity != 0) {
                std::int32_t index = 1;
                do {
                    work.motion_counts[slot] = static_cast<std::int8_t>(
                        static_cast<std::uint8_t>(work.motion_counts[slot]) + 1U);
                    const bool loop = calls.action(slot, work.motion_counts[slot]).identity == 0;
                    const auto link = calls.link_frames(slot, index);
                    const auto action = calls.action(slot, index);
                    auto &textures = calls.textures(slot);
                    calls.set_action(task, action, textures, speed, loop, link);
                    index = work.motion_counts[slot];
                } while (calls.action(slot, index).identity != 0);
            }
            work.selected_motion = 0;
        }
        auto *tail = calls.action_head(primary);
        if (!tail)
            throw Error(Boundary::Actor, slot, "preview event action list is null");
        while (auto *next = calls.next_action(tail))
            tail = next;
        state.tails[slot] = tail;
    }
}
} // namespace sadx::selection_preview
