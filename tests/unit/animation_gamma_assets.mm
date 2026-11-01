// Existing imported Gamma contract only. No animation playback is supplied.
#include "apps/native_gamma_actor_assets.hpp"
#include <iostream>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
}
int main(int argc, char** argv) {
    @autoreleasepool {
        try {
            using namespace sadx;
            require(argc == 2, "expected imported directory");
            const std::filesystem::path directory(argv[1]);
            const auto pack = character_pack::CharacterPack::load(directory / "characters/sonic-selection.actions");
            const auto assets = native::app::load_gamma_actor_assets(directory, pack);
            require(assets.animation_table && assets.animation_table->records.size() == 91, "all 91 table rows");
            require(assets.declarations.size() == 78 &&
                    assets.declarations[61].status == native::ActorResourceStatus::KnownNull,
                    "78 declarations with source null export 61");
            require(assets.declarations[0].status == native::ActorResourceStatus::Loaded &&
                    assets.declarations[0].action.identity == assets.stand.identity &&
                    assets.animation_table->records.at(0).action->id == assets.stand.identity,
                    "request zero is the imported Stand identity");
            require(assets.recipes[72].resource.status == native::ActorResourceStatus::Unavailable &&
                    !assets.recipes[72].resource.boundary.empty() &&
                    !assets.animation_table->records.at(72).action,
                    "constructed row 72 stays explicitly unavailable");
            for (unsigned index = 0; index < 91; ++index) {
                const auto& recipe = assets.recipes[index];
                const auto& row = assets.animation_table->records.at(index);
                const auto status = recipe.resource.status;
                require(row.object_count == recipe.object_count && row.mode == recipe.mode && row.next == recipe.next &&
                        row.transition_step && bits(*row.transition_step) == bits(recipe.transition) &&
                        row.frame_step && bits(*row.frame_step) == bits(recipe.speed), "all scalar metadata survives unavailable payloads");
                if (status == native::ActorResourceStatus::Loaded)
                    require(row.action && row.action->id == recipe.resource.action.identity, "loaded action identity");
                else if (status == native::ActorResourceStatus::KnownNull)
                    require(row.action && !row.action->id, "known null remains known");
                else
                    require(!row.action && !recipe.resource.boundary.empty(), "unavailable is not known null");
                std::cout << index << ' ' << static_cast<unsigned>(status) << ' '
                          << (row.action ? (row.action->id ? 2 : 1) : 0) << ' '
                          << (row.action && row.action->id ? *row.action->id : 0) << ' '
                          << static_cast<unsigned>(*row.object_count) << ' ' << static_cast<int>(*row.mode) << ' '
                          << *row.next << ' ' << bits(*row.transition_step) << ' ' << bits(*row.frame_step) << ' '
                          << recipe.resource.boundary << '\n';
            }
            std::cerr << "91 Gamma imported metadata/status rows characterized; PSetMotion remains unrecovered\n";
            return 0;
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
}
