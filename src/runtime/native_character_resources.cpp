#include "runtime/native_character_resources.hpp"
#include <stdexcept>

namespace sadx::native {
void CharacterResources::initialize() {
    for (std::size_t character = 0; character < texture_assets_.size(); ++character) {
        const auto &declarations = texture_assets_[character];
        if (declarations.empty())
            throw std::runtime_error("missing imported character texture declarations");
        auto &lists = character_textures_[character];
        lists.resize(declarations.size());
        for (std::size_t index = 0; index < lists.size(); ++index) {
            if (declarations[index].texture_count == 0)
                throw std::runtime_error("empty imported character texture declaration");
            lists[index].entries.resize(declarations[index].texture_count);
            character_texture_lists_[character].lists.push_back(&lists[index]);
        }
        character_texture_bindings_[character] = &character_texture_lists_[character];
    }
}
} // namespace sadx::native
