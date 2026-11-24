// Existing imported Miles contract plus its scheduled exec stop. No actor pose is supplied.
#include "apps/native_miles_actor_assets.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_session.hpp"
#include "runtime/native_title_scene.hpp"
#include <iostream>

namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
} // namespace
namespace sadx::native {
// NOTSA: observe the real private Miles callback reaching its named seam.
struct MilesCallbackProbe {
    static void run(Session &session, TitleScene &scene) {
        session.logo_.initialize_heap();
        session.logo_.initialize_tasks();
        auto &scheduler = session.logo_.task_scheduler();
        const auto task = scene.create_task(7, 1, TitleScene::exec_selection_miles);
        player_initialization::establish_zeroed_primary(scheduler, task);
        player_initialization::establish_zeroed_motion(scheduler, task);
        bool stopped = false;
        try {
            task->exec(scheduler, task);
        } catch (const flow::UnrecoveredGameFlowCall &error) {
            stopped = std::string(error.what()) == "native_miles_parameters";
            if (!stopped)
                throw std::runtime_error(std::string("Miles exec stopped at: ") + error.what());
        }
        if (!stopped)
            throw std::runtime_error("Miles exec does not reach its named parameter seam");
    }
};
} // namespace sadx::native
int main(int argc, char **argv) {
    @autoreleasepool {
        try {
            using namespace sadx;
            require(argc == 2, "expected imported directory");
            const std::filesystem::path directory(argv[1]);
            const auto pack = character_pack::CharacterPack::load(
                directory / "characters/sonic-selection.actions");
            const auto assets = native::app::load_miles_actor_assets(directory, pack);
            require(assets.animation_table && assets.animation_table->records.size() == 136,
                    "all 136 table rows");
            require(assets.declarations.size() == 114, "114 action declarations");
            require(assets.declarations[79].status == native::ActorResourceStatus::Loaded &&
                        assets.declarations[79].action.identity == assets.stand.identity &&
                        assets.animation_table->records.at(0).action.identity ==
                            assets.stand.identity,
                    "request zero is the imported Stand identity");
            require(assets.declarations[84].status == native::ActorResourceStatus::KnownNull,
                    "source null declaration");
            require(assets.recipes[89].resource.status ==
                            native::ActorResourceStatus::Unavailable &&
                        !assets.recipes[89].resource.boundary.empty() &&
                        assets.animation_table->records.at(89).action.status ==
                            animation::ResourceStatus::Unavailable,
                    "constructed row 89 stays explicitly unavailable");
            for (unsigned index = 0; index < 136; ++index) {
                const auto &recipe = assets.recipes[index];
                const auto &row = assets.animation_table->records.at(index);
                require(row.object_count == recipe.object_count &&
                            static_cast<std::int8_t>(row.mode) == recipe.mode &&
                            row.next == recipe.next &&
                            bits(row.transition_step) == bits(recipe.transition) &&
                            bits(row.frame_step) == bits(recipe.speed),
                        "recipe rows bind into the animation table");
            }
            require(assets.joins.size() == 30 && assets.joins[0].vertex_pairs != 0 &&
                        assets.join_pairs[0].size() == 8 && assets.joins[29].object == 0,
                    "weld rows and pair arrays bind");
            for (const auto object : assets.tail_objects)
                require(object != 0 &&
                            pack.record(object).kind == character_pack::ResourceKind::Object,
                        "tail object identities bind");
            for (const auto object : assets.sibling_objects)
                require(object != 0 &&
                            pack.record(object).kind == character_pack::ResourceKind::Object,
                        "sibling object identities bind");
            for (const auto model : assets.tail_models)
                require(model != 0 &&
                            pack.record(model).kind == character_pack::ResourceKind::Model,
                        "tail model identities bind");
            require(assets.collision_info.size() == 5, "five collision records");
            const auto stand = pack.resolve(assets.stand);
            const auto motion = pack.motion(stand.motion);
            require(motion.part_count == assets.recipes[0].object_count &&
                        motion.frame_count == 16 && motion.channels == 3 &&
                        motion.interpolation == 2,
                    "Stand motion identity");
            const auto render_assets = native::app::load_assets(directory);
            auto shared_pack = std::make_shared<const character_pack::CharacterPack>(pack);
            frame::ModeState mode;
            native::Display display(directory / "system", mode);
            native::Session session(display.output(), shared_pack, display.frame_state(),
                                    display.arithmetic(), mode, render_assets.character_textures,
                                    1, [](std::uint32_t) {});
            audio::AudioConfiguration configuration{false, false, 0, 0, 0};
            session.attach_cleanup(display, configuration);
            session.load_miles_assets(native::app::load_miles_actor_assets(directory, pack));
            const native::TitleAssets titles{};
            native::TitleScene scene(session, display, titles);
            native::MilesCallbackProbe::run(session, scene);
            std::cout << "Miles assets: 114 declarations, 136 recipes, 30 weld rows; "
                         "exec stops at native_miles_parameters\n";
            return 0;
        } catch (const std::exception &error) {
            std::cerr << "native Miles assets: " << error.what() << '\n';
            return 1;
        }
    }
}
