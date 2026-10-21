#include "game/selection_preview.hpp"

namespace sadx::selection_preview {
void load_textures(std::uint16_t character, TextureView view, Calls &calls) {
    if (character >= view.characters.size())
        throw Error(Boundary::TextureCharacter, character, "character texture declaration is absent");
    const auto declaration = view.characters[character];
    if (!declaration)
        return;
    auto &loaded = view.loaded[character];
    if (loaded == 0) {
        for (std::size_t binding = 0; binding < declaration->size(); ++binding) {
            const auto &entry = (*declaration)[binding];
            const auto textures = entry.textures;
            if (!textures)
                break;
            if (entry.archive)
                calls.load_named_texture(*entry.archive, *textures);
            else
                calls.load_individual_texture(*textures);
        }
    }
    loaded = static_cast<std::int8_t>(static_cast<std::uint8_t>(loaded) + 1U);
}
void release_textures(std::uint16_t character, TextureView view, Calls &calls) {
    if (character >= view.characters.size())
        throw Error(Boundary::TextureCharacter, character, "character texture declaration is absent");
    const auto declaration = view.characters[character];
    if (!declaration)
        return;
    auto &loaded = view.loaded[character];
    if (loaded == 1) {
        for (const auto &binding : *declaration) {
            if (!binding.textures)
                break;
            calls.release_texture(*binding.textures);
        }
    }
    if (loaded > 0)
        --loaded;
}
} // namespace sadx::selection_preview
