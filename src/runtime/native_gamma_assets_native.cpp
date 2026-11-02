#include "runtime/native_session.hpp"

namespace sadx::native {
void bind_gamma_setup_resources(PlayerSystem &players, const CharacterTextureAssets &assets,
    std::array<std::vector<regular_textures::List>, 9> &lists,
    const std::vector<std::uint8_t> &bytes) {
    if (players.gamma_setup_asset)
        throw std::runtime_error("native Gamma setup resources already loaded");
    constexpr std::size_t kGamma = 6, kEffects = 2;
    if (assets[kGamma].size() <= kEffects || lists[kGamma].size() <= kEffects ||
        assets[kGamma][kEffects].archive != "E102EFFECT" ||
        assets[kGamma][kEffects].resource_name != "gamma_effects")
        throw std::runtime_error("native Gamma setup texture declaration is unavailable");
    auto asset = std::make_unique<const gamma_setup::Asset>(
        gamma_setup::decode(bytes, lists[kGamma][kEffects]));
    players.gamma_setup_asset = std::move(asset);
    // Publish once. A later source change to this pointer must survive the next call.
    players.gamma_setup.definition = &players.gamma_setup_asset->definition;
}

void bind_gamma_effects_resources(PlayerSystem &players, const CharacterTextureAssets &assets,
    std::array<std::vector<regular_textures::List>, 9> &lists,
    const std::vector<std::uint8_t> &bytes) {
    if (players.gamma_effects_asset)
        throw std::runtime_error("native Gamma effects resources already loaded");
    constexpr std::size_t kGamma = 6, kHit = 4;
    if (assets[kGamma].size() <= kHit || lists[kGamma].size() <= kHit ||
        assets[kGamma][kHit].archive != "E102HIT" ||
        assets[kGamma][kHit].resource_name != "gamma_hit")
        throw std::runtime_error("native Gamma effects texture declaration is unavailable");
    players.gamma_effects_asset = std::make_unique<gamma_effects::Asset>(
        gamma_effects::decode(bytes, lists[kGamma][kHit]));
}

void Session::load_player_assets(GammaActorAssets assets,
                                std::optional<std::vector<std::uint8_t>> setup_definition,
                                std::optional<std::vector<std::uint8_t>> effects_definition) {
    if (players_.gamma_assets)
        throw std::runtime_error("native player assets already loaded");
    if (setup_definition)
        bind_gamma_setup_resources(players_, character_resources_.assets(),
                                   character_resources_.lists(), *setup_definition);
    if (effects_definition)
        bind_gamma_effects_resources(players_, character_resources_.assets(),
                                     character_resources_.lists(), *effects_definition);
    bind_gamma_resources(players_, character_scene_, assets);
    players_.gamma_assets = std::make_unique<GammaActorAssets>(std::move(assets));
}
} // namespace sadx::native
