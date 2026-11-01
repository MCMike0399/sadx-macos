#include "runtime/native_regular_resources.hpp"

namespace sadx::native {
void RegularResources::attach(RegularTextureAssets assets, std::uint8_t &resync,
                              const std::int32_t &language) {
    if (regular_texture_state_)
        throw std::runtime_error("native regular texture owners already initialized");
    regular_assets_ = std::move(assets);
    regular_resource_lists_.resize(regular_assets_.lists.size());
    for (std::size_t index = 0; index < regular_assets_.lists.size(); ++index)
        regular_resource_lists_[index] = bind_texture_list(regular_assets_.lists[index]);
    regular_groups_.resize(regular_assets_.groups.size());
    const auto bind = [this](const RegularTextureBindingAsset &source) {
        return regular_textures::Binding{
            source.archive,
            source.texture_list ? &regular_resource_lists_.at(*source.texture_list) : nullptr};
    };
    for (std::size_t index = 0; index < regular_groups_.size(); ++index) {
        const auto &source = regular_assets_.groups[index];
        if (source.empty())
            throw std::runtime_error("regular texture group lacks its common descriptor");
        auto &group = regular_groups_[index];
        group.common = bind(source.front());
        for (std::size_t entry = 1; entry < source.size(); ++entry)
            group.regular.push_back(bind(source[entry]));
    }
    for (std::size_t index = 0; index < regular_catalog_.size(); ++index) {
        const auto group = regular_assets_.entries[index];
        regular_catalog_[index] = group ? &regular_groups_.at(*group) : nullptr;
    }
    regular_texture_state_ = std::make_unique<regular_textures::State>(
        regular_textures::State{resync, language, common_textures_, regular_textures_,
                                regular_assets_.localizations, {}, {}});
}
} // namespace sadx::native
