#pragma once
#include "runtime/menu_texture_assets.hpp"
#include "engine/sprite_immediate.hpp"
#include "game/game_mode_services.hpp"

namespace sadx::test {
using native::bind_texture_list;
using native::RegularTextureBindingAsset;
// Frozen native binding baseline from ddac8f7. Bodies below are copied verbatim;
// the test adapter supplies the same vectors and a satisfied regular-state prerequisite.
struct PriorMenuTextureBindings {
    std::vector<sprite::TextureList> menu_resource_lists_;
    std::vector<sprite::TextureList *> menu_list_bindings_;
    native::MenuTextureCatalog menu_texture_catalog_{};
    regular_textures::Binding japanese_title_texture_{};
    bool menu_textures_attached_ = false;
    bool regular_texture_state_ = true;
    std::int32_t text_language_ = 0;
    void attach(const native::MenuTextureAssets &assets,
                std::vector<sprite::TextureList> &regular_resource_lists_) {
    if (menu_textures_attached_ || !regular_texture_state_)
        throw std::runtime_error("native menu textures require initialized regular resource owners");
    menu_resource_lists_.resize(assets.lists.size());
    menu_list_bindings_.resize(assets.lists.size());
    for (std::size_t index = 0; index < assets.lists.size(); ++index) {
        const auto &source = assets.lists[index];
        if (source.regular_texture_list) {
            auto &list = regular_resource_lists_.at(*source.regular_texture_list);
            if (list.entries.size() != source.declaration.texture_count)
                throw std::runtime_error("aliased menu texture declaration count differs");
            menu_list_bindings_[index] = &list;
        } else {
            menu_resource_lists_[index] = bind_texture_list(source.declaration);
            menu_list_bindings_[index] = &menu_resource_lists_[index];
        }
    }
    const auto bind = [this](const RegularTextureBindingAsset &source) {
        return regular_textures::Binding{
            source.archive, source.texture_list ? menu_list_bindings_.at(*source.texture_list) : nullptr};
    };
    for (std::size_t language = 0; language < menu_texture_catalog_.size(); ++language)
        for (std::size_t index = 0; index < menu_texture_catalog_[language].size(); ++index)
            menu_texture_catalog_[language][index] = bind(assets.languages[language][index]);
    japanese_title_texture_ = bind(assets.japanese_title);
    menu_textures_attached_ = true;
}
    const regular_textures::Binding &binding(std::int32_t language, std::uint32_t index) {
        text_language_ = language;
        return lookup(index);
    }
    const regular_textures::Binding &lookup(std::uint32_t index) const {
    if (!menu_textures_attached_ || text_language_ < 0 ||
        static_cast<std::size_t>(text_language_) >= menu_texture_catalog_.size())
        throw flow::UnrecoveredGameFlowCall("native_menu_texture_language");
    return menu_texture_catalog_.at(static_cast<std::size_t>(text_language_)).at(index);
}
    bool attached() const { return menu_textures_attached_; }
    const native::MenuTextureCatalog &catalog() const { return menu_texture_catalog_; }
    const regular_textures::Binding &japanese() const { return japanese_title_texture_; }
};
} // namespace sadx::test
