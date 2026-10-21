#pragma once
#include "apps/native_regular_texture_assets.hpp"
#include "runtime/menu_texture_assets.hpp"

namespace sadx::native::app {
inline MenuTextureAssets load_menu_texture_assets(const std::filesystem::path &directory) {
    const auto source = read_metadata(directory / "menu-textures.json");
    MenuTextureAssets result{};
    id lists = source[@"lists"];
    if (![lists isKindOfClass:NSArray.class] || [lists count] == 0)
        throw std::runtime_error("missing imported menu texture declarations");
    for (id value in lists) {
        const auto record = dictionary(value, "menu texture list");
        MenuTextureListAsset list{texture_declaration(record[@"declaration"]), {}};
        if (record[@"regular_texture_list"] != NSNull.null)
            list.regular_texture_list = static_cast<std::size_t>(integer(
                record[@"regular_texture_list"], 0, UINT32_MAX, "regular texture alias"));
        result.lists.push_back(std::move(list));
    }
    const auto binding = [&](id value) {
        const auto record = dictionary(value, "menu texture binding");
        RegularTextureBindingAsset item{};
        if (record[@"archive"] != NSNull.null)
            item.archive = string(record[@"archive"], "menu texture archive");
        if (record[@"texture_list"] != NSNull.null)
            item.texture_list = static_cast<std::size_t>(integer(
                record[@"texture_list"], 0, result.lists.size() - 1, "menu texture identity"));
        return item;
    };
    const auto languages = array(source[@"languages"], result.languages.size(), "menu languages");
    for (std::size_t language = 0; language < result.languages.size(); ++language) {
        const auto textures =
            array(languages[language], result.languages[language].size(), "menu textures");
        for (std::size_t index = 0; index < result.languages[language].size(); ++index)
            result.languages[language][index] = binding(textures[index]);
    }
    result.japanese_title = binding(source[@"japanese_title"]);
    id unavailable = source[@"unavailable_archives"];
    if (![unavailable isKindOfClass:NSArray.class])
        throw std::runtime_error("invalid unavailable menu archive declaration");
    for (id name in unavailable)
        result.unavailable_archives.push_back(string(name, "unavailable menu archive"));
    return result;
}
} // namespace sadx::native::app
