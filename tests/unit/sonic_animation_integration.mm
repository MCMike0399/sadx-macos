#include "apps/native_gamma_actor_assets.hpp"
#include "apps/native_sonic_animation_assets.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_session.hpp"
#include "runtime/native_title_scene.hpp"
#include "runtime/native_sonic_context.hpp"
#include <cassert>
#include <iostream>
#include <type_traits>

namespace sadx::native {
// NOTSA: exercise the actual callback and observe its canonical owners.
struct SonicCallbackProbe {
    static void run(Session &session, TitleScene &scene, Display &display,
                    const std::filesystem::path &directory,
                    const character_pack::CharacterPack &pack) {
        static_assert(std::is_same_v<decltype(std::declval<const SonicAnimationAssets &>().table()),
                                    const std::shared_ptr<const animation::TableStorage> &>);
        bool rejected = false;
        try { session.load_sonic_animation_assets(nullptr); }
        catch (const std::runtime_error &) { rejected = true; }
        assert(rejected && !session.players_.sonic_animation_assets);
        session.logo_.initialize_heap();
        session.logo_.initialize_tasks();
        auto &scheduler = session.logo_.task_scheduler();
        const auto task = scene.create_task(7, 1, TitleScene::exec_selection_sonic);
        player_initialization::establish_zeroed_primary(scheduler, task);
        player_initialization::establish_zeroed_motion(scheduler, task);
        auto initialization_stop = [&](const char *boundary) {
            const auto cw = display.arithmetic().control_word();
            bool stopped = false;
            try { task->exec(scheduler, task); }
            catch (const sonic::Unavailable &error) {
                stopped = std::string(error.what()) == boundary;
                if (!stopped)
                    std::cerr << "Actual Sonic boundary: " << error.what() << "; expected " << boundary << '\n';
            }
            assert(stopped && display.arithmetic().control_word() == cw);
            assert(session.players_.current.primary == task->primary_work);
            const auto player = session.players_.registry.players[0];
            assert(player && player->stage == player_initialization::Stage::Complete);
            assert(player->physics && player->task == task);
            assert(session.players_.registry.allocations.at(player->allocation.get()) == player);
        };
        initialization_stop("sonic_animation_table");
        // Entry-time publication stays null even though initialization allocated a player.
        assert(!session.players_.current.player);
        auto assets = app::load_sonic_animation_assets(directory, pack);
        const auto *owner = assets.get();
        const auto *table_member = &assets->table();
        const auto *resources = &assets->resources();
        const auto table = assets->table();
        assert(!resources->objects);
        session.load_sonic_animation_assets(std::move(assets));
        assert(!assets && session.players_.sonic_animation_assets.get() == owner);
        assert(&owner->table() == table_member && owner->table() == table);
        assert(&owner->resources() == resources);
        const auto entry_player = session.players_.registry.players[0]->allocation;
        initialization_stop("sonic_LoadCharTextures");
        assert(task->primary_state->action_at_0 == 1); // Source stores Stand before textures.
        assert(session.players_.current.player == entry_player);
        assert(session.players_.registry.players[0]->physics->animation_tail.table_at_140 == table);
        rejected = false;
        try { session.load_sonic_animation_assets(app::load_sonic_animation_assets(directory, pack)); }
        catch (const std::runtime_error &) { rejected = true; }
        assert(rejected && session.players_.sonic_animation_assets.get() == owner);
        rejected = false;
        try { session.load_sonic_animation_assets(nullptr); }
        catch (const std::runtime_error &) { rejected = true; }
        assert(rejected && session.players_.sonic_animation_assets.get() == owner);
        initialization_stop("sonic_controls");
        assert(&owner->resources() == resources && &owner->table() == table_member);
        // The invocation runtime only borrows: repeated callback destruction keeps backing alive.
        for (const auto &entry : resources->key_arrays)
            assert(entry.second.bytes && !entry.second.bytes->empty());
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
        native::SonicCallbackProbe::run(session, scene, display, directory, *pack);
        std::cout << "Session rejects null/replacement; stable Sonic owner/table/resources; actual "
                     "callback initializes player/table/model prefix then reaches texture/control stops\n";
    }
}
