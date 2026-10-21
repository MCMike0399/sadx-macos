#pragma once
#include "apps/native_assets.hpp"
#include "runtime/regular_texture_assets.hpp"

namespace sadx::native::app {
inline RegularTextureListAsset texture_declaration(id value) {
    const auto record = dictionary(value, "texture list declaration");
    RegularTextureListAsset list{};
    list.texture_count = static_cast<std::size_t>(
        integer(record[@"texture_count"], 1, 4096, "texture count"));
    const auto filenames = array(record[@"filenames"], list.texture_count, "texture filenames");
    const auto attributes = array(record[@"attributes"], list.texture_count, "texture attributes");
    for (std::size_t index = 0; index < list.texture_count; ++index) {
        if (filenames[index] == NSNull.null)
            list.filenames.push_back(std::nullopt);
        else
            list.filenames.push_back(string(filenames[index], "texture filename"));
        list.attributes.push_back(static_cast<std::uint32_t>(
            integer(attributes[index], 0, UINT32_MAX, "texture attribute")));
    }
    return list;
}
// NOTSA: resolve imported identities once; no original image pointers enter the runtime.
inline RegularTextureAssets load_regular_texture_assets(const std::filesystem::path &directory) {
    const auto source = read_metadata(directory / "regular-textures.json");
    RegularTextureAssets result{};
    id lists = source[@"lists"];
    id groups = source[@"groups"];
    if (![lists isKindOfClass:NSArray.class] || [lists count] == 0 ||
        ![groups isKindOfClass:NSArray.class] || [groups count] == 0)
        throw std::runtime_error("missing regular texture declarations");
    for (id value in lists)
        result.lists.push_back(texture_declaration(value));
    for (id value in groups) {
        if (![value isKindOfClass:NSArray.class] || [value count] == 0 || [value count] > 33)
            throw std::runtime_error("invalid regular texture group");
        std::vector<RegularTextureBindingAsset> group;
        for (id entry in value) {
            const auto record = dictionary(entry, "regular texture binding");
            RegularTextureBindingAsset binding{};
            if (record[@"archive"] != NSNull.null)
                binding.archive = string(record[@"archive"], "regular texture archive");
            if (record[@"texture_list"] != NSNull.null)
                binding.texture_list = static_cast<std::size_t>(
                    integer(record[@"texture_list"], 0, result.lists.size() - 1, "texture identity"));
            group.push_back(std::move(binding));
        }
        result.groups.push_back(std::move(group));
    }
    const auto entries = array(source[@"entries"], result.entries.size(), "regular texture entries");
    for (std::size_t index = 0; index < result.entries.size(); ++index) {
        if (entries[index] != NSNull.null)
            result.entries[index] = static_cast<std::size_t>(
                integer(entries[index], 0, result.groups.size() - 1, "regular group identity"));
    }
    const auto localizations = array(source[@"localizations"], result.localizations.size(),
                                      "texture localizations");
    for (std::size_t index = 0; index < result.localizations.size(); ++index) {
        const auto rule = dictionary(localizations[index], "texture localization");
        result.localizations[index] = {
            static_cast<std::int32_t>(integer(rule[@"language"], INT32_MIN, INT32_MAX, "language")),
            string(rule[@"source"], "localized texture source"),
            std::make_shared<const std::string>(string(rule[@"replacement"], "texture replacement"))};
    }
    return result;
}
} // namespace sadx::native::app
