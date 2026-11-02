#include "runtime/native_session.hpp"

namespace sadx::native {
// NOTSA: install once so callback resource borrows retain a stable owner.
void Session::load_sonic_animation_assets(std::unique_ptr<SonicAnimationAssets> assets) {
    if (!assets)
        throw std::runtime_error("native Sonic animation assets are null");
    if (players_.sonic_animation_assets)
        throw std::runtime_error("native Sonic animation assets already loaded");
    players_.sonic_animation_assets = std::move(assets);
}
} // namespace sadx::native
