#include "runtime/native_session.hpp"

namespace sadx::native {
void Session::load_player_assets(GammaActorAssets assets) {
    if (players_.gamma_assets)
        throw std::runtime_error("native player assets already loaded");
    bind_gamma_resources(players_, character_scene_, assets);
    players_.gamma_assets = std::make_unique<GammaActorAssets>(std::move(assets));
}
} // namespace sadx::native
