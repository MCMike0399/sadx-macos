#include "runtime/big_motion.hpp"
#include "player/animation/animation_content.hpp"

namespace sadx::native {
BigMotionAssets::BigMotionAssets(const character_pack::CharacterPack &pack,
                                 const BigActorAssets &assets) {
    for (const auto &declaration : assets.declarations)
        if (declaration.status == animation::ResourceStatus::Loaded)
            import_animation_action(pack, declaration.action, resources_);
    for (const auto &recipe : assets.recipes)
        if (recipe.resource.status == animation::ResourceStatus::Loaded)
            import_animation_action(pack, recipe.resource.action, resources_);
}
} // namespace sadx::native
