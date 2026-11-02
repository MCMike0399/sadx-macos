#include "game/selection_preview.hpp"
#include "player/player_task.hpp"
#include <cassert>
#include <iostream>

namespace p = sadx::selection_preview;
namespace {
struct MissingEventEngine {};
struct Context final : sadx::TaskCallbackContext, p::ActionCalls {
    p::State preview;
    p::ActionState actions;
    std::uint8_t selected = 0;
    std::int32_t counter = 0;
    sadx::regular_textures::List list{};
    unsigned reached_actions = 0;
    std::shared_ptr<sadx::PlayerTaskState> primary(const sadx::TaskHandle &task) override {
        return task->primary_state;
    }
    void set_input(std::uint8_t player, std::int8_t mode) override {
        const auto work = std::static_pointer_cast<p::Work>(preview.task->auxiliary_native_work);
        const auto state = work->actors[player]->primary_state;
        assert(state && state->secondary_mode_at_1);
        state->secondary_mode_at_1 = static_cast<std::uint8_t>(mode);
        state->orientation.flags |= 4096;
    }
    std::int32_t &miles_counter() override { return counter; }
    sadx::ActionHandle action(std::size_t, std::int32_t) override { return {71}; }
    sadx::regular_textures::List &textures(std::size_t) override { return list; }
    std::int32_t link_frames(std::size_t, std::int32_t) override { return 0; }
    void set_action(const sadx::TaskHandle &task, sadx::ActionHandle action,
                    sadx::regular_textures::List &textures, float speed,
                    bool loop, std::int32_t link) override {
        assert(task->primary_state->secondary_mode_at_1 == 12);
        assert((task->primary_state->orientation.flags & 4096) != 0);
        assert(action.identity == 71 && &textures == &list && speed == 0.25f && loop && link == 0);
        ++reached_actions;
        throw MissingEventEngine{};
    }
    sadx::PlayerEventActionNode *action_head(const std::shared_ptr<sadx::PlayerTaskState> &) override {
        assert(false); // Must never reach list reads after the explicit lower stop.
        return nullptr;
    }
    sadx::PlayerEventActionNode *next_action(sadx::PlayerEventActionNode *) override {
        assert(false);
        return nullptr;
    }
};
void preview_callback(sadx::TaskScheduler &scheduler, const sadx::TaskHandle &task) {
    const auto context = std::static_pointer_cast<Context>(scheduler.callback_context(preview_callback));
    const auto work = std::static_pointer_cast<p::Work>(task->auxiliary_native_work);
    p::update_actions(*work, context->actions, context->selected, *context);
}
}
int main() {
    sadx::TaskScheduler scheduler;
    const auto context = std::make_shared<Context>();
    assert(scheduler.register_callback_context(preview_callback, context));
    context->preview.task = scheduler.append_task(3, preview_callback).task;
    const auto first = std::make_shared<p::Work>();
    context->preview.task->auxiliary_native_work = first;
    p::select_motion(context->preview, 257);
    assert(first->selected_motion == 1);
    const auto current = std::make_shared<p::Work>();
    context->preview.task->auxiliary_native_work = current;
    p::select_motion(context->preview, 258);
    assert(current->selected_motion == 2 && first->selected_motion == 1);
    const auto actor = scheduler.append_task(1, nullptr).task;
    actor->primary_state = std::make_shared<sadx::PlayerTaskState>();
    actor->primary_state->secondary_mode_at_1 = 0;
    current->actors[0] = actor;
    bool stopped = false;
    try { scheduler.run_index(3); }
    catch (const MissingEventEngine &) { stopped = true; }
    assert(stopped && context->reached_actions == 1);
    assert(current->motion_counts[0] == 0); // Failed idle queue is not published as started.
    assert(scheduler.destroy_task(context->preview.task));
    context->preview.task.reset();
    bool absent = false;
    try { p::select_motion(context->preview, 1); }
    catch (const p::Error &error) { absent = error.boundary() == p::Boundary::Task; }
    assert(absent);
    context->preview.task = scheduler.append_task(3, preview_callback).task;
    context->preview.task->auxiliary_native_work = std::make_shared<p::Work>();
    const auto reentry = std::static_pointer_cast<p::Work>(context->preview.task->auxiliary_native_work);
    assert(reentry->warmup_frames == 0 && reentry->selected_motion == 0);
    assert((reentry->motion_counts == std::array<std::int8_t, 7>{}));
    std::cout << "preview actions: live task/work, scheduler lower stop, cancel/reentry passed\n";
}
