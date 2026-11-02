#include "runtime/gamma_motion.hpp"

namespace sadx::native {
GammaMotionAssets::GammaMotionAssets(const character_pack::CharacterPack &pack,
                                     const GammaActorAssets &assets) {
    for (const auto &declaration : assets.declarations)
        if (declaration.status == animation::ResourceStatus::Loaded)
            import_animation_action(pack, declaration.action, resources_);
    // Includes the source's constructed action, whose declaration is not an export.
    for (const auto &recipe : assets.recipes)
        if (recipe.resource.status == animation::ResourceStatus::Loaded)
            import_animation_action(pack, recipe.resource.action, resources_);
}
animation::Outcome update_gamma_motion(animation::View view, TaskWorkArena &arena,
                                       const GammaMotionAssets &assets,
                                       const character_pack::ObjectState &objects,
                                       fidelity::Context &fp) {
    animation::Runtime runtime{&arena, assets.resources(), &objects};
    return animation::update(view, runtime, fp);
}
} // namespace sadx::native
