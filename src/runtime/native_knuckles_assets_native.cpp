#include "runtime/native_session.hpp"

namespace sadx::native {
void Session::load_knuckles_assets(KnucklesActorAssets assets) {
    if (players_.knuckles_assets)
        throw std::runtime_error("native Knuckles assets already loaded");
    auto motion = std::make_unique<KnucklesMotionAssets>(*character_pack_, assets);
    // Publish first: weld rows borrow the published pair arrays.
    players_.knuckles_assets = std::make_unique<KnucklesActorAssets>(std::move(assets));
    players_.knuckles_motion_assets = std::move(motion);
    bind_knuckles_resources(players_, character_scene_, *players_.knuckles_assets);
}
} // namespace sadx::native
