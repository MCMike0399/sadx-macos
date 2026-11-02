#include "apps/native_gamma_actor_assets.hpp"
#include "apps/native_sonic_animation_assets.hpp"
#include "player/animation/animation_native_access.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_session.hpp"
#include "runtime/native_title_scene.hpp"
#include "runtime/native_sonic_fields.hpp"
#include "runtime/native_sonic_context.hpp"
#include <cassert>
#include <iostream>

namespace sadx::native {
// NOTSA: observes the real callback compiled with the exact pending Root hunk.
struct SonicCallbackProbe {
    static void run(Session &session, TitleScene &scene, Display &display,
                    const std::filesystem::path &directory, unsigned scenario) {
        auto assets = app::load_sonic_animation_assets(directory, *session.character_pack_);
        if (scenario == 2) {
            // Same own animation declarations, genuinely absent model composition.
            assets = std::make_unique<SonicAnimationAssets>(*session.character_pack_,
                                                            assets->declarations());
        }
        session.load_sonic_animation_assets(std::move(assets));
        auto *model = session.players_.sonic_animation_assets->model();
        auto &objects = session.character_scene_.objects;
        if (model) {
            assert(&model->objects() == &objects);
            bool rejected = false;
            try { model->bind_scene(objects); }
            catch (const std::runtime_error &) { rejected = true; }
            assert(rejected);
            assert(model->state().joins.rows.size() == 37);
            for (const auto &row : model->state().joins.rows) {
                assert(!row.original_vertices);
                if (row.vertex_pairs)
                    assert(row.vertex_pairs->size() == 2U * row.vertex_count);
            }
            if (scenario == 1 || scenario == 3)
                objects.object(model->root()).sibling = 0;
        }
        const auto prior = model ? objects.object(model->root()) : character_pack::ObjectResource{};
        session.soft_reset_.metal_sonic = scenario == 1 ? 128 : 0;
        session.logo_.initialize_heap();
        session.logo_.initialize_tasks();
        auto &scheduler = session.logo_.task_scheduler();
        auto arena = scheduler.work_arena();
        const auto task = scene.create_task(7, 1, TitleScene::exec_selection_sonic);
        player_initialization::establish_zeroed_primary(scheduler, task);
        player_initialization::establish_zeroed_motion(scheduler, task);
        const auto primary = task->primary_work;
        const auto control = display.arithmetic().control_word();
        std::string boundary;
        try { task->exec(scheduler, task); }
        catch (const sonic::Unavailable &error) { boundary = error.what(); }
        std::cerr << "scenario " << scenario << " boundary " << boundary << "\n";
        assert(boundary == (scenario == 3 ? "sonic_PJoinVertexes" :
                            scenario == 2 ? "sonic_model_graph" : "sonic_LoadCharTextures"));
        assert(display.arithmetic().control_word() == control);
        const auto player = session.players_.registry.players[0];
        assert(player && player->stage == player_initialization::Stage::Complete);
        assert(session.players_.current.primary == primary);
        assert(!session.players_.current.player); // actual entry alias; no post-init refresh
        assert(player->physics->animation_tail.table_at_140 ==
               session.players_.sonic_animation_assets->table());
        assert(sonic::native_fields::player(*player->physics).requested_animation == 0);
        if (!model)
            return;
        auto &root = objects.object(model->root());
        assert(root.model == prior.model && root.child == prior.child);
        assert(root.position == prior.position && root.rotation == prior.rotation && root.scale == prior.scale);
        assert(root.evaluation_flags == prior.evaluation_flags);
        animation::Runtime runtime{arena.get(), session.players_.sonic_animation_assets->resources(), &objects};
        animation::detail::NativeAccess consumer;
        const auto borrowed = consumer.object({model->root()}, runtime, animation::Phase::SelectNewObject);
        assert(borrowed.native == &root && !borrowed.reference);
        assert(consumer.sibling(borrowed, animation::Phase::SelectNewObject).id.value_or(0) == root.sibling);
        if (scenario == 3) {
            assert(root.sibling == model->attach());
            assert(!model->state().model_sibling); // cache store is after first join call
            assert(player->physics->animation_tail.join_table == &model->state().joins);
            assert(model->state().joins.rows[11].destination_object == &objects.object(model->attach()));
            assert(!collision_component::component(*arena, primary));
            assert(player_initialization::bind_primary_view(task).mode == 0);
            assert(sonic::native_fields::player(*player->physics).join_mode == 0);
        } else {
            if (scenario == 0)
                assert(prior.sibling == model->attach());
            assert(root.sibling == prior.sibling);
            assert(model->state().model_sibling == prior.sibling);
            assert(player->physics->animation_tail.join_table &&
                   !*player->physics->animation_tail.join_table);
            const auto collision = collision_component::component(*arena, primary);
            assert(collision && collision->info_count == 2 && collision->info.size() == 2);
            assert(collision->info[0].a == player->parameters.height);
            assert(collision->info[1].a == model->collision()[1].a);
            assert(player_initialization::bind_primary_view(task).mode == 1);
        }
    }
};
} // namespace sadx::native

int main(int argc, char **argv) {
    @autoreleasepool {
        assert(argc == 2);
        using namespace sadx;
        const std::filesystem::path directory(argv[1]);
        const auto assets = native::app::load_assets(directory);
        auto pack = std::make_shared<const character_pack::CharacterPack>(
            character_pack::CharacterPack::load(directory / "characters/sonic-selection.actions"));
        const auto metadata = native::app::read_metadata(directory / "characters/sonic-actor.json");
        NSData *encoded = [NSJSONSerialization dataWithJSONObject:metadata options:0 error:nil];
        for (unsigned fault = 0; fault < 6; ++fault) {
            NSMutableDictionary *changed = [NSJSONSerialization JSONObjectWithData:encoded
                options:NSJSONReadingMutableContainers error:nil];
            NSMutableArray *objects = changed[@"objects"];
            NSMutableDictionary *joins = changed[@"joins"];
            NSMutableArray *rows = joins[@"sonic"];
            switch (fault) {
            case 0: objects[0][@"object"] = @0; break;
            case 1: objects[1][@"object"] = @(pack->record_count() + 1); break;
            case 2: objects[1][@"object"] = @(pack->action(character_pack::SelectionAction::SonicIdle).identity); break;
            case 3: rows[36][@"object"] = objects[0][@"object"]; break;
            case 4: rows[0][@"vertex_pairs"] = @0; break;
            case 5: rows[0][@"original_vertices"] = @1; break;
            }
            bool rejected = false;
            try { (void)native::app::load_sonic_model_assets(changed, *pack); }
            catch (const std::exception &) { rejected = true; }
            assert(rejected);
        }
        for (unsigned scenario = 0; scenario < 4; ++scenario) {
            frame::ModeState mode;
            native::Display display(directory / "system", mode);
            native::Session session(display.output(), pack, display.frame_state(), display.arithmetic(),
                                    mode, assets.character_textures, 1, [](std::uint32_t) {});
            session.load_player_assets(native::app::load_gamma_actor_assets(directory, *pack));
            const native::TitleAssets titles{};
            native::TitleScene scene(session, display, titles);
            native::SonicCallbackProbe::run(session, scene, display, directory, scenario);
        }
        std::cout << "Real callback plus exact Root hunk: first normal collision/texture boundary; live Metal skip; changed-sibling splice/PJoin boundary; absent-model boundary; canonical aliases and animation scene consumer\n";
    }
}
