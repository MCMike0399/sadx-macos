#include "apps/native_gamma_actor_assets.hpp"
#include "apps/native_regular_texture_assets.hpp"
#include "apps/native_sonic_animation_assets.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_session.hpp"
#include "runtime/native_sonic_context.hpp"
#include "runtime/native_sonic_fields.hpp"
#include "runtime/native_title_scene.hpp"
#include <cassert>
#include <iostream>

namespace {
using namespace sadx;
TaskWorkHandle leave_arena_space(TaskWorkArena &arena, std::uint32_t remaining) {
    const auto head = arena.free_head();
    assert(head);
    for (const auto &block : arena.blocks()) {
        if (block.offset != *head)
            continue;
        assert(block.physical_next);
        const auto available = *block.physical_next - block.offset;
        const auto filler = arena.allocate(available - remaining - 32);
        assert(filler);
        return filler.work;
    }
    assert(false);
    return {};
}
struct ExhaustionCalls final : sonic_model::InitializationCalls {
    unsigned scenario;
    unsigned motions = 0;
    bool callbacks = false;
    PlayerJoinTable *published = nullptr;
    sonic_model::State state;
    std::vector<unsigned> trace;
    explicit ExhaustionCalls(unsigned scenario) : scenario(scenario) {
        state.joins.rows.push_back({});
    }
    void exhaust(unsigned flags, bool setup) {
        auto arena = std::make_shared<TaskWorkArena>();
        TaskScheduler scheduler;
        assert(scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}));
        const auto filler = leave_arena_space(*arena, scenario == 3 ? 96 : 0);
        const auto before = scheduler.source_graph();
        bool absent = false;
        bool threw = false;
        try {
            if (setup)
                absent = !sonic_model::load_collision_task({1, 2, 3}, 7, scheduler, *arena);
            else
                absent = !sonic_model::create_initialization_task(
                    scheduler, flags, 3,
                    flags == 2 ? sonic_model::initialize_effect : sonic_model::initialize_jiggle);
        } catch (...) {
            threw = true;
        }
        assert(!threw && absent);
        const auto after = scheduler.source_graph();
        assert(after.nodes.empty() && after.free_slots == before.free_slots);
        assert(arena->release(filler));
        const auto reusable = scheduler.create_element(flags, 3, nullptr);
        assert(reusable && reusable.task->primary_work);
        assert(bool(reusable.task->motion_work) == bool(flags & 1));
    }
    void load_textures(std::uint8_t character) override { trace.push_back(10 + character); }
    std::uint8_t metal() override { return 0; }
    void clear_free_word(unsigned index) override { trace.push_back(20 + index); }
    std::uint8_t player_index() override { return 7; }
    void load_collision_task(std::uint8_t index) override {
        assert(index == 7);
        trace.push_back(30);
        if (scenario == 0)
            exhaust(2, true);
    }
    void set_motion() override {
        ++motions;
        trace.push_back(40);
    }
    std::int16_t stage() override { return 0; }
    std::uint16_t act() override { return 0; }
    character_pack::ObjectResource *join_target() override { return nullptr; }
    void publish_join_table(PlayerJoinTable &table) override {
        published = &table;
        trace.push_back(50);
    }
    void join_vertices() override { trace.push_back(60); }
    void create_effect(unsigned flags) override {
        trace.push_back(70 + flags);
        if ((scenario == 1 && flags == 2) || (scenario >= 2 && flags == 3))
            exhaust(flags, false);
    }
    void install_callbacks() override {
        callbacks = true;
        trace.push_back(80);
    }
};
void check_creation_exhaustion() {
    for (unsigned scenario = 0; scenario < 4; ++scenario) {
        ExhaustionCalls calls(scenario);
        sonic_model::initialize_resources(calls.state, calls);
        const std::vector<unsigned> expected{10, 20, 21, 30, 40, 50, 60, 72, 73, 80, 40};
        assert(calls.trace == expected);
        assert(calls.motions == 2 && calls.callbacks && calls.published == &calls.state.joins);
    }
    for (unsigned failure = 0; failure < 3; ++failure) {
        auto arena = std::make_shared<TaskWorkArena>();
        TaskScheduler scheduler;
        if (failure != 0)
            assert(scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}));
        if (failure == 1)
            arena->set_payload_transport({}, [](std::uint32_t, const void *, std::uint32_t) {
                return false;
            });
        if (failure == 2) {
            leave_arena_space(*arena, 96);
            arena->set_payload_transport([](std::uint32_t, void *, std::uint32_t) {
                return false;
            }, {});
        }
        bool threw = false;
        try {
            sonic_model::create_initialization_task(scheduler, 3, 3, nullptr);
        } catch (...) {
            threw = true;
        }
        assert(threw);
    }
    // Distinguish failed rollback from a retired task before checking the helper.
    // This exercises the guard independently of an unavailable payload write.
    for (bool translate : {false, true}) {
        auto arena = std::make_shared<TaskWorkArena>();
        TaskScheduler scheduler;
        assert(scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}));
        leave_arena_space(*arena, 96);
        arena->set_payload_transport([](std::uint32_t, void *, std::uint32_t) {
            return false;
        }, {});
        if (!translate) {
            const auto failed = scheduler.create_element(3, 3, nullptr);
            assert(failed.failure == TaskFailure::DetachedCreationRollbackRejected);
            assert(failed.step == TaskStep::CreationRollback);
            assert(failed.work_failure == TaskWorkFailure::Exhausted);
            assert(failed.task && !failed.task->retired());
        } else {
            bool threw = false;
            try {
                sonic_model::create_initialization_task(scheduler, 3, 3, nullptr);
            } catch (...) {
                threw = true;
            }
            assert(threw);
        }
    }
    // A successfully retired task is insufficient outside primary/motion work.
    for (unsigned flags : {6U, 10U}) {
        auto arena = std::make_shared<TaskWorkArena>();
        TaskScheduler scheduler;
        assert(scheduler.initialize_source_pool(arena, std::shared_ptr<void>{}));
        leave_arena_space(*arena, 96);
        bool threw = false;
        try {
            sonic_model::create_initialization_task(scheduler, flags, 3, nullptr);
        } catch (...) {
            threw = true;
        }
        assert(threw);
    }
    std::cout << "Four rolled-back exhaustion paths retain call/publication order; "
                 "uninitialized, unavailable-write and rollback failures remain failures\n";
}
} // namespace

namespace sadx::native {
struct SonicCallbackProbe {
    static void run(Session &session, TitleScene &scene, Display &display,
                    const std::filesystem::path &directory, unsigned scenario) {
        auto assets = app::load_sonic_animation_assets(directory, *session.character_pack_);
        session.load_sonic_animation_assets(std::move(assets));
        auto *model = session.players_.sonic_animation_assets->model();
        assert(model && &model->objects() == &session.character_scene_.objects);
        assert(model->state().metal_joins.rows.size() == 8);
        session.soft_reset_.metal_sonic = scenario == 1 ? 128 : 0;
        if (scenario == 2)
            model->objects().object(model->root()).sibling = 0;
        session.logo_.initialize_heap();
        session.logo_.initialize_tasks();
        auto &scheduler = session.logo_.task_scheduler();
        const auto task = scene.create_task(7, 1, TitleScene::exec_selection_sonic);
        player_initialization::establish_zeroed_primary(scheduler, task);
        player_initialization::establish_zeroed_motion(scheduler, task);
        const auto primary = task->primary_work;
        const auto motion = task->motion_work;
        CharacterTextureViews texture_views(session.character_resources_.assets(),
                                            session.character_resources_.lists());
        auto services = session.regular_texture_services();
        auto catalog = texture_views.catalog(session.character_resources_.loaded());
        if (scenario == 3) {
            regular_textures::load_character(0, catalog, *session.regular_resources_.state(),
                                             services);
            session.character_resources_.loaded()[0] = 37;
        }
        const auto loaded = session.character_resources_.loaded();
        std::vector<std::pair<texture_lifetime::Memory *, std::uint16_t>> retained_textures;
        for (const auto &list : session.character_resources_.lists()[0])
            for (auto *memory : list.entries)
                if (memory)
                    retained_textures.emplace_back(memory, memory->references);
        bool lower_seen = false;
        player_initialization::PlayerHandle captured_record;
        std::shared_ptr<PlayerPhysicsState> captured_physics;
        TaskWorkHandle captured_allocation;
        const auto load_archive = services.load_archive;
        services.load_archive = [&](std::string_view name) {
            if (!lower_seen) {
                lower_seen = true;
                captured_record = session.players_.registry.players[0];
                assert(captured_record);
                captured_physics = captured_record->physics;
                captured_allocation = captured_record->allocation;
                const std::uint32_t sentinels[2] = {0x12345678, 0x89abcdef};
                assert(scheduler.work_arena()->write(captured_allocation, 148, sentinels, 8));
                player_initialization::bind_primary_view(task).player_index = 3;
                if (scenario == 4 || scenario == 6)
                    session.soft_reset_.metal_sonic = 128;
                if (scenario == 5)
                    throw sonic::Unavailable("controlled_missing_texture_archive");
                if (scenario == 7)
                    player_initialization::bind_primary_view(task).status |= 256;
                if (scenario == 6) {
                    captured_record->physics =
                        std::make_shared<PlayerPhysicsState>(*captured_physics);
                    captured_record->physics->animation_tail.join_table = nullptr;
                    const auto replacement = scheduler.work_arena()->allocate(512);
                    assert(replacement);
                    captured_record->allocation = replacement.work;
                    const std::uint32_t other[2] = {0x24681357, 0xfdb97531};
                    assert(scheduler.work_arena()->write(replacement.work, 148, other, 8));
                }
            } else if ((scenario == 4 || scenario == 6) &&
                       name == session.character_resources_.assets()[8][0].archive) {
                session.soft_reset_.metal_sonic = 0;
            }
            return load_archive(name);
        };
        std::string boundary;
        try {
            if (scenario < 4) {
                task->exec(scheduler, task);
            } else {
                // The same native entry with a reentrant real lower texture loader.
                sonic::Context context{session.players_, scheduler, task, display.arithmetic()};
                context.stage = &session.stage_;
                context.act = &session.act_;
                context.scene = &session.character_scene_;
                context.alternate_model = &session.soft_reset_.metal_sonic;
                context.parameters = &session.players_.gamma_assets->parameters;
                context.events = &session.bootstrap_.event_work;
                context.auxiliary = TitleScene::initialize_player_auxiliary;
                context.model = model;
                context.animation_table = &session.players_.sonic_animation_assets->table();
                animation::Runtime animation{scheduler.work_arena().get(),
                                             session.players_.sonic_animation_assets->resources(),
                                             &session.character_scene_.objects};
                context.animation = &animation;
                context.character_textures = catalog;
                context.texture_state = session.regular_resources_.state();
                context.texture_services = &services;
                sonic::bind_event_work(context);
                sonic::bind_editor_state(context,
                                         {session.editor_status_, session.mission_editor_status_});
                sonic::run(context);
            }
        } catch (const sonic::Unavailable &error) {
            boundary = error.what();
        }
        std::cerr << "scenario " << scenario << " boundary " << boundary << "\n";
        const auto player = session.players_.registry.players[0];
        if (scenario == 5) {
            assert(boundary == "controlled_missing_texture_archive");
            assert(lower_seen && player == captured_record && player->physics == captured_physics);
            assert(session.character_resources_.loaded() == loaded);
            std::uint32_t prefix[2]{};
            assert(scheduler.work_arena()->read(captured_allocation, 148, prefix, 8));
            assert(prefix[0] == 0x12345678 && prefix[1] == 0x89abcdef);
            assert(scheduler.list_head(2) &&
                   scheduler.list_head(2)->exec == TitleScene::initialize_player_auxiliary);
            assert(!player->physics->animation_tail.buffer_at_148.value());
            return;
        }
        assert(boundary == (scenario == 7 ? "sonic_common_remaining_tail" :
                scenario == 1 || scenario == 2 ? "sonic_join_vertices_CalcMMMatrix"
                                                           : "sonic_common_model_slot_20"));
        auto physics = captured_physics ? captured_physics : player->physics;
        assert(player && player->stage == player_initialization::Stage::Complete);
        assert(task->primary_work == primary && task->motion_work == motion);
        assert(session.players_.current.primary == primary && !session.players_.current.player);
        assert(physics->animation_tail.table_at_140 ==
               session.players_.sonic_animation_assets->table());
        assert(player_initialization::bind_primary_view(task).mode == 1);
        assert(session.character_resources_.loaded()[0] == loaded[0] + 1);
        assert(session.character_resources_.loaded()[8] ==
               loaded[8] + (scenario == 1 || scenario == 4 || scenario == 6));
        std::uint32_t words[2] = {1, 2};
        assert(scheduler.work_arena()->read(
            (captured_allocation ? captured_allocation : player->allocation), 148, words, 8));
        assert(words[0] == 0 && words[1] == 0);
        auto direct = scheduler.list_head(2);
        assert(direct && direct->exec == sonic_model::collision_task);
        assert(direct->motion_work && direct->motion_work->requested_size() == 136);
        const auto work =
            std::static_pointer_cast<sonic_model::CollisionTaskWork>(direct->auxiliary_native_work);
        assert(work && work->allocation == direct->motion_work);
        assert(work->objects == model->state().direct_ahead);
        assert(player_initialization::bind_primary_view(direct).player_index ==
               (scenario >= 4 ? 3 : 0));
        for (auto slot : {0U, 8U}) {
            if (slot == 8 && scenario != 1 && scenario != 4 && scenario != 6)
                continue;
            for (unsigned index = 0; index < session.character_resources_.lists()[slot].size();
                 ++index) {
                const auto &list = session.character_resources_.lists()[slot][index];
                assert(!list.entries.empty());
                for (auto *memory : list.entries)
                    assert(memory && memory->references && memory->global_index >= 0 &&
                           memory->surface);
                const auto &asset = session.character_resources_.assets()[slot][index];
                const auto again = display.output().load_pvm_archive(asset.archive);
                assert(again == list.entries);
                texture_lifetime::List retained{again, false, {}, {}};
                display.release_texture_list(retained);
            }
        }
        if (scenario == 3) {
            assert(!lower_seen);
            for (const auto &[memory, references] : retained_textures)
                assert(memory->references == references);
        }
        if (scenario >= 4) {
            assert(lower_seen && player == captured_record);
            assert(physics->animation_tail.join_table == &model->state().joins);
            if (scenario == 6) {
                assert(player->physics != physics);
                assert(player->allocation != captured_allocation);
                std::uint32_t other[2]{};
                assert(scheduler.work_arena()->read(player->allocation, 148, other, 8));
                assert(other[0] == 0x24681357 && other[1] == 0xfdb97531);
                assert(player->physics->animation_tail.join_table &&
                       !*player->physics->animation_tail.join_table);
            }
        }
        assert(physics->animation_tail.buffer_at_148 && *physics->animation_tail.buffer_at_148);
        for (const auto callback :
             {sonic_model::collision_task, sonic_model::initialize_effect,
              sonic_model::initialize_jiggle, sonic_model::destroy, sonic_model::display}) {
            bool stopped = false;
            try {
                callback(scheduler, task);
            } catch (const sonic::Unavailable &) {
                stopped = true;
            }
            assert(stopped);
        }
        if (scenario != 2) {
            assert(task->display == sonic_model::display &&
                   task->destructor == sonic_model::destroy);
        }
        std::cout << "callback " << scenario << " " << boundary << " loaded "
                  << int(session.character_resources_.loaded()[0]) << " "
                  << int(session.character_resources_.loaded()[8]) << "\n";
    }
};
} // namespace sadx::native

int main(int argc, char **argv) {
    @autoreleasepool {
        assert(argc == 2);
        check_creation_exhaustion();
        using namespace sadx;
        const std::filesystem::path directory(argv[1]);
        auto assets = native::app::load_assets(directory);
        auto pack = std::make_shared<const character_pack::CharacterPack>(
            character_pack::CharacterPack::load(directory / "characters/sonic-selection.actions"));
        for (unsigned scenario = 0; scenario < 8; ++scenario) {
            frame::ModeState mode;
            native::Display display(directory / "system", mode);
            native::Session session(display.output(), pack, display.frame_state(),
                                    display.arithmetic(), mode, assets.character_textures, 1,
                                    [](std::uint32_t) {});
            session.attach_cleanup(display, assets.audio);
            session.attach_regular_textures(native::app::load_regular_texture_assets(directory));
            session.load_player_assets(native::app::load_gamma_actor_assets(directory, *pack));
            const native::TitleAssets titles{};
            native::TitleScene scene(session, display, titles);
            native::SonicCallbackProbe::run(session, scene, display, directory, scenario);
        }
    }
}
