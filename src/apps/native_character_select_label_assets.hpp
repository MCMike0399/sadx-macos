#pragma once
#include "apps/native_assets.hpp"

namespace sadx::native::app {
// NOTSA: only the new label declaration is owned here; names borrow FileSelect's table.
inline std::array<std::uint8_t, 7> load_character_label_textures(
    const std::filesystem::path &directory) {
    const auto source = read_metadata(directory / "character-select-labels.json");
    const auto bindings = dictionary(source[@"menu_textures"], "selection label menu textures");
    (void)integer(bindings[@"labels"], 14, 14, "selection label texture list");
    (void)integer(bindings[@"names"], 15, 15, "selection name texture list");
    (void)integer(bindings[@"metal_name"], 28, 28, "selection Metal texture list");
    std::array<std::uint8_t, 7> result{};
    const auto indices = array(source[@"label_textures"], result.size(), "selection label textures");
    for (std::size_t index = 0; index < result.size(); ++index)
        result[index] = static_cast<std::uint8_t>(
            integer(indices[index], 0, UINT8_MAX, "selection label texture index"));
    return result;
}
} // namespace sadx::native::app
