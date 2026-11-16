// Existing imported Big contract only. No actor behavior is supplied.
#include "apps/native_big_actor_assets.hpp"
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
            const auto assets = native::app::load_big_actor_assets(directory, pack);
            require(assets.animation_table && assets.animation_table->records.size() == 105,
                    "all 105 table rows");
            require(assets.declarations.size() == 90 &&
                        assets.declarations[3].status == native::ActorResourceStatus::KnownNull,
                    "90 declarations with source null rows");
            require(assets.declarations[23].status == native::ActorResourceStatus::Loaded &&
                        assets.declarations[23].action.identity == assets.stand.identity &&
                        assets.animation_table->records.at(0).action.identity == assets.stand.identity,
                    "request zero is the imported Stand identity");
            require(assets.recipes[96].resource.status ==
                            native::ActorResourceStatus::Unavailable &&
                        !assets.recipes[96].resource.boundary.empty() &&
                        assets.animation_table->records.at(96).action.status ==
                            animation::ResourceStatus::Unavailable,
                    "constructed row 96 stays explicitly unavailable");
            for (unsigned index = 0; index < 105; ++index) {
                const auto &recipe = assets.recipes[index];
                const auto &row = assets.animation_table->records.at(index);
                require(row.object_count == recipe.object_count &&
                            static_cast<std::int8_t>(row.mode) == recipe.mode &&
                            row.next == recipe.next && bits(row.transition_step) == bits(recipe.transition) &&
                            bits(row.frame_step) == bits(recipe.speed),
                        "recipe rows bind into the animation table");
            }
            require(assets.joins.size() == 17 && assets.joins[0].vertex_pairs != 0 &&
                        assets.join_pairs[0].size() == 12 && assets.joins[16].object == 0,
                    "weld rows and pair arrays bind");
            require(assets.collision_info.size() == 3, "three collision records");
            const auto stand = pack.resolve(assets.stand);
            const auto motion = pack.motion(stand.motion);
            require(motion.part_count == 73 && motion.frame_count == 32 && motion.channels == 3 &&
                        motion.interpolation == 2,
                    "Stand motion identity");
            std::cout << "Big assets: 90 declarations, 105 recipes, 17 weld rows\n";
            return 0;
        } catch (const std::exception &error) {
            std::cerr << "native Big assets: " << error.what() << '\n';
            return 1;
        }
    }
}
