#include "runtime/native_menu_texture_bindings.hpp"
#include "game/game_mode_services.hpp"

namespace sadx::native {
void MenuTextureBindings::attach(const MenuTextureAssets &assets,
                                 std::vector<sprite::TextureList> &regular) {
    if (attached_)
        throw std::runtime_error("native menu textures require initialized regular resource owners");
    lists_.resize(assets.lists.size());
    bindings_.resize(assets.lists.size());
    for (std::size_t index = 0; index < assets.lists.size(); ++index) {
        const auto &source = assets.lists[index];
        if (source.regular_texture_list) {
            auto &list = regular.at(*source.regular_texture_list);
            if (list.entries.size() != source.declaration.texture_count)
                throw std::runtime_error("aliased menu texture declaration count differs");
            bindings_[index] = &list;
        } else {
            lists_[index] = bind_texture_list(source.declaration);
            bindings_[index] = &lists_[index];
        }
    }
    const auto bind = [this](const RegularTextureBindingAsset &source) {
        return regular_textures::Binding{
            source.archive, source.texture_list ? bindings_.at(*source.texture_list) : nullptr};
    };
    for (std::size_t language = 0; language < catalog_.size(); ++language)
        for (std::size_t index = 0; index < catalog_[language].size(); ++index)
            catalog_[language][index] = bind(assets.languages[language][index]);
    japanese_ = bind(assets.japanese_title);
    attached_ = true;
}
const regular_textures::Binding &MenuTextureBindings::binding(std::int32_t language,
                                                             std::uint32_t index) const {
    if (!attached_ || language < 0 ||
        static_cast<std::size_t>(language) >= catalog_.size())
        throw flow::UnrecoveredGameFlowCall("native_menu_texture_language");
    return catalog_.at(static_cast<std::size_t>(language)).at(index);
}
} // namespace sadx::native
