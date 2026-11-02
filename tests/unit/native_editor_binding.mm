#include "apps/native_gamma_actor_assets.hpp"
#include "apps/native_sonic_animation_assets.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_session.hpp"
#include "runtime/native_sonic_context.hpp"
#include "runtime/native_title_scene.hpp"
#include <cassert>
#include <fstream>
#include <iostream>

namespace sadx::native {
// NOTSA: observe production callback/cleanup borrowing in an isolated native world.
struct SonicCallbackProbe {
    static void run(Session &session, TitleScene &scene, Display &display, std::ostream &out) {
        assert(session.editor_status_ == 0 && session.mission_editor_status_ == 0);
        assert(session.soft_reset_.metal_sonic == 0);
        session.logo_.initialize_heap();
        session.logo_.initialize_tasks();
        auto &scheduler = session.logo_.task_scheduler();
        const auto task = scene.create_task(7, 1, TitleScene::exec_selection_sonic);
        player_initialization::establish_zeroed_primary(scheduler, task);
        player_initialization::establish_zeroed_motion(scheduler, task);
        const auto primary = task->primary_work;
        const auto motion = task->motion_work;
        auto invoke = [&](const char *expected) {
            const auto cw = display.arithmetic().control_word();
            std::string boundary;
            try { task->exec(scheduler, task); }
            catch (const sonic::Unavailable &error) { boundary = error.what(); }
            assert(boundary == expected);
            assert(display.arithmetic().control_word() == cw);
            assert(session.players_.current.primary == primary);
            assert(task->primary_work == primary && task->motion_work == motion);
        };
        invoke("sonic_model_graph");
        const auto player = session.players_.registry.players[0];
        assert(player && player->stage == player_initialization::Stage::Complete);
        assert(!session.players_.current.player); // captured before PInitialize
        assert(player->task == task && player->motion == task->motion_state);
        assert(player->physics->animation_tail.table_at_140 ==
               session.players_.sonic_animation_assets->table());
        assert(player->physics->requested_animation_at_124 == 0);
        assert(scheduler.work_arena()->reference(motion, 0).work == player->allocation);
        assert(session.players_.registry.allocations.at(player->allocation.get()) == player);

        sonic::Context reads{session.players_, scheduler, task, display.arithmetic()};
        const editor::View state{session.editor_status_, session.mission_editor_status_};
        sonic::bind_editor_state(reads, state);
        gamma_selection::Context gamma{session.players_, scheduler, session.bootstrap_, display, task,
            session.stage_, session.act_, nullptr, nullptr, nullptr, {}, nullptr, nullptr,
            session.sprites_, session.character_scene_};
        bool missing = false;
        try { (void)gamma_selection::check_edit_mode(gamma); }
        catch (const flow::UnrecoveredGameFlowCall &error) {
            missing = std::string(error.what()) == "native_gamma_check_edit_mode";
        }
        assert(missing);
        gamma.editor_state = &state;
        constexpr std::array<std::uint16_t, 9> values{0, 1, 2, 0x100, 0x7fff, 0x8000,
                                                    0x8001, 0xfffe, 0xffff};
        const auto saved_stage = session.stage_;
        const auto saved_reset = session.reset_status_;
        session.soft_reset_.neighbor = 0xa7;
        session.soft_reset_.metal_sonic = 0x83;
        for (auto editor : values) {
            for (auto mission : values) {
                session.editor_status_ = editor;
                session.mission_editor_status_ = mission;
                assert(reads.read_editor() == static_cast<std::int16_t>(mission));
                assert(reads.read_forced() == static_cast<std::int16_t>(editor));
                assert(gamma_selection::check_edit_mode(gamma) == editor::check_mode(state));
                task->primary_state->action_at_0 = 86;
                invoke("sonic_unrecovered_body");
                assert(task->primary_state->action_at_0 == (editor || mission ? 87 : 86));
                assert(session.players_.current.player == player->allocation);
                assert(session.players_.registry.players[0] == player);
                assert(session.editor_status_ == editor && session.mission_editor_status_ == mission);
                assert(session.stage_ == saved_stage && session.reset_status_ == saved_reset);
                assert(session.soft_reset_.neighbor == 0xa7 && session.soft_reset_.metal_sonic == 0x83);
                out << editor << '\t' << mission << '\t' << reads.read_editor() << '\t'
                    << reads.read_forced() << '\t' << gamma_selection::check_edit_mode(gamma) << '\n';
            }
        }
        // Missing composition stays explicit; the first nonzero read cannot mask the second.
        unsigned first_reads = 0;
        reads.read_editor = [&] { ++first_reads; return std::int16_t{-1}; };
        reads.read_forced = {};
        task->primary_state->action_at_0 = 86;
        std::string boundary;
        try { sonic::run(reads); }
        catch (const sonic::Unavailable &error) { boundary = error.what(); }
        assert(boundary == "sonic_check_edit_mode_forced" && first_reads == 1);
        unsigned second_reads = 0;
        reads.read_editor = {};
        reads.read_forced = [&] { ++second_reads; return std::int16_t{1}; };
        boundary.clear();
        try { sonic::run(reads); }
        catch (const sonic::Unavailable &error) { boundary = error.what(); }
        assert(boundary == "sonic_check_edit_mode_editor" && second_reads == 0);

        // Exercise Session's unchanged cleanup mapping, without any reset reserved cohort.
        session.editor_status_ = 0x8001;
        session.mission_editor_status_ = 0x42a5;
        session.reset_status_ = 0x91827364;
        audio::AudioConfiguration configuration{};
        session.attach_cleanup(display, configuration);
        try { session.reset_game(); }
        catch (const std::exception &error) {
            std::cout << "Actual cleanup downstream boundary: " << error.what() << '\n';
        }
        assert(session.editor_status_ == 0 && session.reset_status_ == 0);
        assert(session.mission_editor_status_ == 0x42a5);
        assert(reads.task == task); // observer never redirects the task's entry identity
        std::cout << "81 production callback editor pairs; shared Gamma signed results; missing "
                     "read order; initialized player/table; actual cleanup uses canonical word\n";
    }
};
} // namespace sadx::native
int main(int argc, char **argv) {
    @autoreleasepool {
        assert(argc == 3);
        using namespace sadx;
        const std::filesystem::path directory(argv[1]);
        std::ofstream output(argv[2]);
        assert(output);
        const auto assets = native::app::load_assets(directory);
        auto pack = std::make_shared<const character_pack::CharacterPack>(
            character_pack::CharacterPack::load(directory / "characters/sonic-selection.actions"));
        frame::ModeState mode;
        native::Display display(directory / "system", mode);
        native::Session session(display.output(), pack, display.frame_state(), display.arithmetic(),
                                mode, assets.character_textures, 1, [](std::uint32_t) {});
        session.load_player_assets(native::app::load_gamma_actor_assets(directory, *pack));
        session.load_sonic_animation_assets(native::app::load_sonic_animation_assets(directory, *pack));
        const native::TitleAssets titles{};
        native::TitleScene scene(session, display, titles);
        native::SonicCallbackProbe::run(session, scene, display, output);
    }
}
