#include "runtime/native_session.hpp"

namespace sadx::native {
void Session::load_big_assets(BigActorAssets assets) {
    if (players_.big_assets)
        throw std::runtime_error("native Big assets already loaded");
    auto motion = std::make_unique<BigMotionAssets>(*character_pack_, assets);
    // Publish first: weld rows borrow the published pair arrays.
    players_.big_assets = std::make_unique<BigActorAssets>(std::move(assets));
    players_.big_motion_assets = std::move(motion);
    bind_big_resources(players_, character_scene_, *players_.big_assets);
}
} // namespace sadx::native
