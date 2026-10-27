#include "game/character_textures.hpp"
#include <limits>

namespace sadx::regular_textures {
void load_character(std::size_t character, CharacterCatalog catalog, State &state,
                    Services &services) {
    // NOTSA: validate the imported view before indexing native storage.
    if (character >= catalog.count)
        throw std::out_of_range("character texture slot is outside the imported catalog");
    // Capture the table once: lower loaders may change the catalog's slot.
    const auto table = catalog.tables[character];
    if (table.entries == nullptr)
        return;
    auto &loaded = catalog.loaded[character];
    if (loaded == 0) {
        for (std::size_t index = 0; index < table.count; ++index) {
            const auto &binding = table.entries[index];
            if (binding.textures == nullptr)
                break;
            if (binding.archive)
                load_archive(*binding.archive, binding.textures, state, services);
            else
                load_no_name(*binding.textures, state, services);
            // The next entry is read only after the lower loader returns.
        }
        constexpr std::size_t kAmy = 5; // Characters_Amy
        if (character == kAmy)
            load_egg_robo_textures(state, services);
    }
    // Original quirk: INC wraps the live byte, including negative loaded counts.
    loaded = loaded == std::numeric_limits<std::int8_t>::max()
                 ? std::numeric_limits<std::int8_t>::min()
                 : static_cast<std::int8_t>(loaded + 1);
}
} // namespace sadx::regular_textures
