#include "apps/native_gamma_actor_assets.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_session.hpp"
#include "runtime/native_title_scene.hpp"
#include "runtime/native_sonic_context.hpp"
#include "runtime/native_event_actions_context.hpp"
#include <cassert>
#include <iostream>

namespace sadx::native {
// NOTSA: observe the real private callback and canonical owners without replacing it.
struct SonicCallbackProbe {
    static void run(Session &session, TitleScene &scene, Display &display) {
        session.logo_.initialize_heap();
        session.logo_.initialize_tasks();
        auto &scheduler = session.logo_.task_scheduler();
        auto arena = scheduler.work_arena();
        const auto task = scene.create_task(7, 1, TitleScene::exec_selection_sonic);
        player_initialization::establish_zeroed_primary(scheduler, task);
        player_initialization::establish_zeroed_motion(scheduler, task);
        const auto primary = task->primary_work;
        auto &flags = task->primary_state->orientation.flags;
        flags = 0;
        const auto cw = display.arithmetic().control_word();
        bool stopped = false;
        try { task->exec(scheduler, task); }
        catch (const sonic::Unavailable &error) {
            stopped = std::string(error.what()) == "sonic_check_edit_mode_editor";
        }
        assert(stopped);
        assert(session.players_.current.primary == primary);
        assert(!session.players_.current.player);
        assert(session.players_.registry.allocations.empty());
        assert(display.arithmetic().control_word() == cw);
        player_initialization::NativeAccess access(session.players_.gamma_assets->parameters,
            session.bootstrap_.event_work, TitleScene::initialize_player_auxiliary);
        player_initialization::PlayerHandle player;
        player_initialization::initialize(0, task,
            player_initialization::bind_existing_motion(task), scheduler,
            session.players_.registry, access, display.arithmetic(), player);
        assert(player && player->stage == player_initialization::Stage::Complete);
        stopped = false;
        try { task->exec(scheduler, task); }
        catch (const sonic::Unavailable &error) {
            stopped = std::string(error.what()) == "sonic_check_edit_mode_editor";
        }
        assert(stopped && session.players_.current.primary == primary);
        assert(session.players_.current.player == player->allocation);
        assert(session.players_.registry.allocations.at(player->allocation.get()) == player);

        // Bind real event storage, then replace the task primary. All three
        // callbacks must still use the captured allocation, including later writes.
        const auto fields = event_primary_fields();
        EventActionsContext events(*arena, session.event_actions_, session.players_.registry,
                                  *session.character_pack_, fields);
        event_actions::initialize(events.core, primary);
        sonic::Context context{session.players_, scheduler, task, display.arithmetic()};
        sonic::bind_event_work(context);
        const auto original_event = event_actions::event_work(*arena, primary);
        const auto action = session.character_pack_->action(character_pack::StartupAction::ClearPose);
        auto &textures = session.character_resources_.lists()[0][0];
        event_actions::append_action(events.core, primary, action, &textures, 0.25f, 1, 0);
        assert(event_actions::action_head(*arena, primary));
        assert(!context.read_event_link()); // action head is not the Sonic path link
        const auto other = scheduler.create_element(7, 1, nullptr).task;
        player_initialization::establish_zeroed_primary(scheduler, other);
        event_actions::initialize(events.core, other->primary_work);
        task->primary_work = other->primary_work;
        task->primary_state = other->primary_state;
        context.write_event_mode(7);
        assert(context.read_event_mode() == 7);
        assert(event_actions::move_mode(*arena, primary) == 7);
        assert(event_actions::move_mode(*arena, other->primary_work) == 0);
        const auto path = arena->allocate(8).work;
        assert(arena->set_reference(original_event, 76, path)); // eventwk.path.list
        assert(context.read_event_link());
        event_actions::clear_actions(events.core, primary);
        assert(!event_actions::action_head(*arena, primary));
        assert(context.read_event_link());
        assert(arena->set_reference(original_event, 76, {}));
        assert(!context.read_event_link());
        assert(arena->release(original_event));
        stopped = false;
        try { context.write_event_mode(3); }
        catch (const event_actions::Error &error) {
            stopped = error.boundary == event_actions::Boundary::EventMode;
        }
        assert(stopped);
    }
};
}
int main(int argc, char **argv) {
    @autoreleasepool {
        assert(argc == 2);
        using namespace sadx;
        const std::filesystem::path directory(argv[1]);
        const auto assets = native::app::load_assets(directory);
        auto pack = std::make_shared<const character_pack::CharacterPack>(
            character_pack::CharacterPack::load(directory / "characters/sonic-selection.actions"));
        frame::ModeState mode;
        native::Display display(directory / "system", mode);
        native::Session session(display.output(), pack, display.frame_state(), display.arithmetic(),
                                mode, assets.character_textures, 1, [](std::uint32_t) {});
        session.load_player_assets(native::app::load_gamma_actor_assets(directory, *pack));
        const native::TitleAssets titles{};
        native::TitleScene scene(session, display, titles);
        native::SonicCallbackProbe::run(session, scene, display);
        std::cout << "Real Sonic callback publishes canonical current owners before the editor stop; "
                     "captured event mode/path bindings survive primary replacement and reject release\n";
    }
}
