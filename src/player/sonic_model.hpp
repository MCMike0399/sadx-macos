#pragma once
#include "engine/character_pack.hpp"
#include "player/player_join_vertices.hpp"

namespace sadx::sonic_model {
using ObjectId = character_pack::ResourceId;
// NOTSA: session-lifetime storage for the source's shared Sonic model state.
// Imported objects remain owned by the character scene, including sibling edits.
struct State {
    // GLOBAL: SONIC 0x3c55e28 SonicWeldInfo size=888
    PlayerJoinTable joins;
    // GLOBAL: SONIC 0x3c55d40 SonicModelSibling size=4
    ObjectId model_sibling = 0;
};
// NOTSA: references into the actual initialized player and primary owners.
struct Player {
    PlayerJoinTable *&joins;
    std::int16_t &join_mode;
    std::uint16_t &idle_timer;
    std::uint8_t &mode;
};
// NOTSA: helper composition, not a replacement implementation of either callee.
// PJoinVertexes retains its single provenance declaration in gamma_selection_core.hpp.
struct Calls {
    virtual ~Calls() = default;
    virtual void join_vertices() = 0;
    virtual void initialize_collision() = 0;
    virtual void store_collision_height() = 0;
};
// NOTSA: inline SonicTheHedgehog model initialization prefix, ending before
// LoadCharTextures. This is not a source function entry or a complete actor update.
void initialize_prefix(State &, character_pack::ObjectState &, ObjectId attach,
                       ObjectId root, Player, bool alternate, Calls &);
} // namespace sadx::sonic_model
