#include "runtime/native_session.hpp"

namespace sadx::native {
void Session::load_amy_assets(AmyActorAssets assets) {
    if (players_.amy_assets)
        throw std::runtime_error("native Amy assets already loaded");
    auto motion = std::make_unique<AmyMotionAssets>(*character_pack_, assets);
    // Publish first: weld rows borrow the published pair arrays.
    players_.amy_assets = std::make_unique<AmyActorAssets>(std::move(assets));
    players_.amy_motion_assets = std::move(motion);
    bind_amy_resources(players_, character_scene_, *players_.amy_assets);
}
} // namespace sadx::native
