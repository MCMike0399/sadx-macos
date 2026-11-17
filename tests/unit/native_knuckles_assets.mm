// Existing imported Knuckles contract only. No actor behavior is supplied.
#include "apps/native_knuckles_actor_assets.hpp"
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
int main(int argc, char **argv) {
    @autoreleasepool {
        try {
            using namespace sadx;
            require(argc == 2, "expected imported directory");
            const std::filesystem::path directory(argv[1]);
            const auto pack = character_pack::CharacterPack::load(
                directory / "characters/sonic-selection.actions");
            const auto assets = native::app::load_knuckles_actor_assets(directory, pack);
            require(assets.animation_table && assets.animation_table->records.size() == 115,
                    "all 115 table rows");
            require(assets.declarations.size() == 90, "90 action declarations");
            require(assets.declarations[82].status == native::ActorResourceStatus::Loaded &&
                        assets.declarations[82].action.identity == assets.stand.identity &&
                        assets.animation_table->records.at(0).action.identity ==
                            assets.stand.identity,
                    "request zero is the imported Stand identity");
            require(assets.declarations[8].status == native::ActorResourceStatus::KnownNull,
                    "source null declaration");
            require(assets.recipes[36].resource.status ==
                            native::ActorResourceStatus::Unavailable &&
                        !assets.recipes[36].resource.boundary.empty() &&
                        assets.animation_table->records.at(36).action.status ==
                            animation::ResourceStatus::Unavailable,
                    "constructed row 36 stays explicitly unavailable");
            for (unsigned index = 0; index < 115; ++index) {
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
            require(assets.collision_info.size() == 5, "five collision records");
            const auto stand = pack.resolve(assets.stand);
            const auto motion = pack.motion(stand.motion);
            require(motion.part_count == assets.recipes[0].object_count &&
                        motion.frame_count == 32 && motion.channels == 3 &&
                        motion.interpolation == 2,
                    "Stand motion identity");
            std::cout << "Knuckles assets: 90 declarations, 115 recipes, 30 weld rows\n";
            return 0;
        } catch (const std::exception &error) {
            std::cerr << "native Knuckles assets: " << error.what() << '\n';
            return 1;
        }
    }
}
