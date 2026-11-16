#include "runtime/native_session.hpp"

namespace sadx::native {
void Session::load_miles_assets(MilesActorAssets assets) {
    if (players_.miles_assets)
        throw std::runtime_error("native Miles assets already loaded");
    auto motion = std::make_unique<MilesMotionAssets>(*character_pack_, assets);
    // Publish first: weld rows borrow the published pair arrays.
    players_.miles_assets = std::make_unique<MilesActorAssets>(std::move(assets));
    players_.miles_motion_assets = std::move(motion);
    bind_miles_resources(players_, character_scene_, *players_.miles_assets);
}
} // namespace sadx::native
