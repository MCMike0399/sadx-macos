#pragma once
#include "runtime/regular_texture_assets.hpp"
#include "runtime/native_menu_texture_bindings.hpp"
#include "game/game_mode_services.hpp"

namespace sadx::test {
using namespace native;
// Frozen e5cf19d native Session attachment body and fields, not a Windows oracle.
// The reduced host replaces only Display/LogoRenderer transport with a callback spy.
struct ArchiveOutput {
    using Loader = std::function<void(std::string_view, sprite::TextureList &)>;
    Loader loader;
    std::function<void()> binding;
    void bind_archive_loader(Loader value) {
        if (binding) binding();
        loader = std::move(value);
    }
    ArchiveOutput &output() { return *this; }
};
struct PriorRegularResources {
    struct { std::uint8_t resync = 0; } loop_;
    std::int32_t text_language_ = 0;
    ArchiveOutput *display_ = nullptr;
    regular_textures::Services services;
    regular_textures::Services regular_texture_services() {
        if (!display_)
            throw flow::UnrecoveredGameFlowCall("native_regular_texture_display");
        return services;
    }
    sprite::TextureList *common_textures_ = nullptr;
    std::array<sprite::TextureList *, 32> regular_textures_{};
    RegularTextureAssets regular_assets_{};
    std::vector<sprite::TextureList> regular_resource_lists_;
    std::vector<regular_textures::Group> regular_groups_;
    regular_textures::Catalog regular_catalog_{};
    std::unique_ptr<regular_textures::State> regular_texture_state_;
void attach_regular_textures(RegularTextureAssets assets) {
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
        regular_textures::State{loop_.resync, text_language_, common_textures_, regular_textures_,
                                regular_assets_.localizations, {}, {}});
    if (!display_)
        throw flow::UnrecoveredGameFlowCall("native_regular_texture_display");
    display_->output().bind_archive_loader([this](std::string_view name, sprite::TextureList &list) {
        auto services = regular_texture_services();
        regular_textures::load_archive(name, &list, *regular_texture_state_, services);
    });
}
    sprite::TextureList *&common() { return common_textures_; }
    std::array<sprite::TextureList *, 32> &regular() { return regular_textures_; }
    std::vector<sprite::TextureList> &lists() { return regular_resource_lists_; }
    const auto &groups() const { return regular_groups_; }
    const auto &catalog() const { return regular_catalog_; }
    const auto &assets() const { return regular_assets_; }
    regular_textures::State *state() { return regular_texture_state_.get(); }
};
} // namespace sadx::test
