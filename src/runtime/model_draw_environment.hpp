#pragma once
#include "engine/character_geometry.hpp"
#include "engine/normal_uv_strip.hpp"
#include "game/game_bootstrap.hpp"
#include "game/stage_lighting.hpp"

namespace sadx::native {
// NOTSA: renderer inputs borrow the scene, bootstrap hook and lighting system owners.
struct ModelDrawEnvironment {
    const character_pack::CharacterPack &pack;
    character_pack::SceneState &scene;
    const bootstrap::MaterialCallback &material_callback;
    const lighting::State &lighting;
};
class Display;
// NOTSA: the canonical buffer owner is required at the resource boundary.
polybuff::BufferState &normal_uv_buffer(Display &display);
} // namespace sadx::native
