#pragma once
#include "engine/basic_model.hpp"

namespace sadx::basic_model {
// Source table entry is one RET, shared by every polygon slot and other Ninja callbacks.
// FUNCTION: SONIC 0x78c5a0 NinjaReturnOnly
void return_only_primitive();
// FUNCTION: SONIC 0x784ae0 njDrawModel_
void draw_dynamic_model(character_pack::ResourceId, const character_pack::CharacterPack &,
                        const character_pack::MaterialState &, Owners, Context &);
// NOTSA: selects the reached immediate renderer leaf, sharing existing device owners.
void draw_dynamic_mesh(Context &, VertexLayout, Primitive, const Meshset &,
                       character_pack::ResourceId points, character_pack::ResourceId normals);
} // namespace sadx::basic_model
