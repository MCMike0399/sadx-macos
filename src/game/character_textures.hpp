#pragma once

#include "game/regular_textures.hpp"
#include <cstddef>
#include <cstdint>

namespace sadx::regular_textures {
// NOTSA: a PVM descriptor view borrowing archive text and the texture-list owner.
// Absence of archive means the source descriptor has no filename.
struct CharacterBinding {
    std::optional<std::string_view> archive;
    List *textures;
};
// NOTSA: borrowed imported PVM entries. A null entries pointer means no source
// table; a nonnull pointer with count zero represents an empty source table.
// Count bounds readable entries; a null textures member also terminates. Native
// imports may omit the sentinel. Callbacks must keep entries/text alive and keep
// termination inside the borrowed capacity (or at its implicit end).
struct CharacterTable {
    const CharacterBinding *entries;
    std::size_t count;
};
// NOTSA: borrows the session's catalog and signed-byte loaded owners. Tables and
// counters must cover count character slots and remain alive through lower calls.
struct CharacterCatalog {
    const CharacterTable *tables;
    std::int8_t *loaded;
    std::size_t count;
};
// STUB: SONIC 0x4b3da0 LoadEggRoboTextures awaiting recovery
void load_egg_robo_textures(State &state, Services &services);
// FUNCTION: SONIC 0x421280 LoadCharTextures
void load_character(std::size_t character, CharacterCatalog catalog, State &state,
                    Services &services);
} // namespace sadx::regular_textures
