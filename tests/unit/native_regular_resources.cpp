#include "regular_resource_baseline.hpp"
#ifndef BASELINE_ONLY
#include "runtime/native_regular_resources.hpp"
#endif
#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <type_traits>

namespace {
using namespace sadx;
#ifndef BASELINE_ONLY
// Same native Session transport suffix as the frozen host, with the new owner.
struct CurrentRegularResources {
    struct { std::uint8_t resync = 0; } loop_;
    std::int32_t text_language_ = 0;
    test::ArchiveOutput *display_ = nullptr;
    regular_textures::Services services;
    native::RegularResources resources;
    regular_textures::Services regular_texture_services() {
        if (!display_)
            throw flow::UnrecoveredGameFlowCall("native_regular_texture_display");
        return services;
    }
    void attach_regular_textures(native::RegularTextureAssets assets) {
        resources.attach(std::move(assets), loop_.resync, text_language_);
        if (!display_)
            throw flow::UnrecoveredGameFlowCall("native_regular_texture_display");
        display_->output().bind_archive_loader([this](std::string_view name, sprite::TextureList &list) {
            auto calls = regular_texture_services();
            regular_textures::load_archive(name, &list, *resources.state(), calls);
        });
    }
    auto &common() { return resources.common(); }
    auto &regular() { return resources.regular(); }
    auto &lists() { return resources.lists(); }
    const auto &catalog() const { return resources.catalog(); }
    auto *state() { return resources.state(); }
};
#endif
native::RegularTextureAssets assets() {
    native::RegularTextureAssets result{};
    result.lists = {{2, {"first", "other"}, {10, 11}}, {1, {"second"}, {12}}};
    result.groups = {{{"COMMON", 0}, {"CON_REGULAR", 1}, {"ALIAS", 0}, {std::nullopt, std::nullopt}},
                     {{std::nullopt, std::nullopt}}};
    result.entries = {0, 0, 1};
    result.localizations = {{{1, "first", std::make_shared<const std::string>("localized")}}};
    return result;
}
template<class F> std::string error(F action) {
    try { action(); return "completed"; }
    catch (const std::exception &failure) { return failure.what(); }
}
template<class Host> std::string snapshot(Host &host) {
    std::ostringstream out;
    const auto id = [&](const auto *list) {
        if (!list) return -1;
        for (std::size_t i = 0; i < host.lists().size(); ++i)
            if (list == &host.lists()[i]) return static_cast<int>(i);
        return -2;
    };
    out << bool(host.state()) << ':' << id(host.common()) << ':';
    for (const auto *list : host.regular()) out << id(list) << ',';
    for (const auto &list : host.lists()) {
        out << " L" << list.entries.size();
        for (auto attr : list.attributes) out << '/' << attr;
        for (const auto &name : list.filenames) out << '/' << (name ? *name : "<null>");
    }
    for (const auto *group : host.catalog()) {
        if (!group) { out << " null"; continue; }
        out << " G" << id(group->common.textures);
        for (const auto &binding : group->regular)
            out << '/' << binding.archive.value_or("<absent>") << '=' << id(binding.textures);
    }
    if (auto *state = host.state()) {
        assert(&state->resync == &host.loop_.resync);
        assert(&state->language == &host.text_language_);
        assert(&state->common == &host.common());
        assert(&state->regular == &host.regular());
        out << " S" << int(state->resync) << '/' << state->language << '/'
            << state->scratch_attributes[0] << '/' << state->cache.was_loading;
    }
    return out.str();
}
template<class Host> std::vector<std::string> exercise() {
    std::vector<std::string> rows;
    for (int failure = 0; failure < 9; ++failure) {
        Host host;
        test::ArchiveOutput output;
        rows.push_back(snapshot(host));
        auto input = assets();
        if (failure != 0) host.display_ = &output;
        if (failure == 1) input.lists[1].attributes.clear();
        if (failure == 2) input.groups[1].clear();
        if (failure == 3) input.groups[0][2].texture_list = 99;
        if (failure == 4) input.entries[1] = 99;
        if (failure == 5) output.binding = [] { throw std::runtime_error("binding failed"); };
        if (failure == 6) input.groups[0][0].texture_list = 99;
        unsigned binding_calls = 0;
        if (failure != 5) output.binding = [&] {
            ++binding_calls;
            assert(host.state());
            assert(host.catalog()[0] == host.catalog()[1]);
            host.text_language_ = 4;
            host.loop_.resync = 7;
            host.state()->scratch_attributes[0] = 123;
            rows.push_back(snapshot(host));
        };
        rows.push_back(error([&] { host.attach_regular_textures(std::move(input)); }));
        rows.push_back(snapshot(host));
        assert(binding_calls == (failure >= 7 ? 1 : 0));
        if (host.state()) {
            const auto *published = host.state();
            auto *list = &host.lists()[0];
            rows.push_back(error([&] { host.attach_regular_textures(assets()); }));
            assert(host.state() == published && &host.lists()[0] == list);
        } else {
            // Retry preserves the original partially appended groups, not rollback.
            output.binding = {};
            rows.push_back(error([&] { host.attach_regular_textures(assets()); }));
            rows.push_back(snapshot(host));
            assert(host.state());
        }
        if (failure < 7) continue;
        native::MenuTextureBindings menu;
        native::MenuTextureAssets menu_input{};
        menu_input.lists = {{assets().lists[0], 0}};
        for (auto &language : menu_input.languages)
            for (auto &binding : language) binding = {"menu", 0};
        menu_input.japanese_title = {"japanese", 0};
        menu.attach(menu_input, host.lists());
        assert(menu.binding(0, 0).textures == &host.lists()[0]);
        host.text_language_ = 1;
        host.loop_.resync = 0;
        auto &state = *host.state();
        state.cache.was_loading = true;
        auto cached = std::make_shared<int>(1);
        std::weak_ptr<int> weak = cached;
        state.cache.archives.push_back(cached);
        cached.reset();
        texture_lifetime::Memory memory{};
        host.services = {
            [&](std::string_view name) {
                rows.push_back(std::string(name) + ":" + std::to_string(host.loop_.resync));
                assert(host.loop_.resync == 1);
                assert(weak.expired());
                return regular_textures::ArchiveResult{1, {&memory}};
            },
            [&](auto &list) {
                assert(*list.filenames[0] == "localized");
                assert(host.loop_.resync == 1);
                rows.push_back("individual-localized");
                return 19;
            }, [](std::string_view) {}};
        // The stored callback reads services at invocation, and uses the same State.
        output.loader("callback", host.lists()[0]);
        assert(menu.japanese().textures->entries[0] == &memory);
        assert(host.lists()[0].attributes[0] == 123);
        regular_textures::load_no_name(host.lists()[0], state, host.services);
        assert(*host.lists()[0].filenames[0] == "first");
        regular_textures::load_regular(0, host.catalog(), state, host.services);
        assert(host.common() == &host.lists()[0]);
        assert(host.regular()[0] == &host.lists()[1]);
        assert(host.regular()[1] == &host.lists()[0]);
        assert(!host.regular()[2]);
        rows.push_back(snapshot(host));
    }
    return rows;
}
} // namespace
int main() {
    const auto expected = exercise<sadx::test::PriorRegularResources>();
#ifdef BASELINE_ONLY
    for (const auto &row : expected) std::cout << row << '\n';
#else
    static_assert(!std::is_move_constructible_v<sadx::native::RegularResources>);
    static_assert(!std::is_copy_constructible_v<sadx::native::RegularResources>);
    assert(expected == exercise<CurrentRegularResources>());
    std::cout << expected.size() << " native baseline observations match; live aliases, callback and partial ordering pass.\n";
#endif
}
