#pragma once
#include "game/save_data.hpp"

namespace sadx::save {
// NOTSA: the two maintained save records, shared as one owner type by runtime worlds.
// Current is the first SDK SaveData record; the other SDK array entries are not modeled.
// Source extents describe encoded records, never host Data padding.
struct Profiles {
    // GLOBAL: SONIC 0x3b2b3a8 CurrentSaveData size=1392
    Data current = {};
    // GLOBAL: SONIC 0x3b29d70 MenuSaveBackup size=1392
    Data menu_backup = {};
};
} // namespace sadx::save
