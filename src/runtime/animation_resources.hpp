#pragma once
#include "engine/character_pack.hpp"
#include "player/animation/animation.hpp"

namespace sadx::native {
// NOTSA: shared adapter for validated MDATA2 payloads; owns immutable keys only.
void import_animation_action(const character_pack::CharacterPack &, ActionHandle,
                             animation::Resources &);
} // namespace sadx::native
