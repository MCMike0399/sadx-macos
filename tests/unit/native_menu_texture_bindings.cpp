#include "menu_texture_binding_baseline.hpp"
#ifndef BASELINE_ONLY
#include "runtime/native_menu_texture_bindings.hpp"
#endif
#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
using namespace sadx;
using List = sprite::TextureList;
native::RegularTextureListAsset declaration(std::size_t count, std::string name) {
    native::RegularTextureListAsset result{count, {}, {}};
    for (std::size_t i = 0; i < count; ++i) {
        result.filenames.push_back(name + std::to_string(i));
        result.attributes.push_back(static_cast<std::uint32_t>(100 + i));
    }
    return result;
}
native::MenuTextureAssets assets() {
    native::MenuTextureAssets result{};
    result.lists = {{declaration(2, "owned-a"), {}}, {declaration(3, "owned-b"), {}},
                    {declaration(2, "ignored-alias"), 0}};
    // Only alias count matters. Its unused declaration payload must stay ignored.
    result.lists[2].declaration.filenames.clear();
    result.lists[2].declaration.attributes.clear();
    for (std::size_t language = 0; language < result.languages.size(); ++language)
        for (std::size_t slot = 0; slot < result.languages[language].size(); ++slot)
            result.languages[language][slot] = {
                slot % 10 == 0 ? std::optional<std::string>{}
                               : "archive-" + std::to_string(language) + "-" + std::to_string(slot),
                slot % 3};
    result.japanese_title = {"japanese", 2};
    return result;
}
template<class Owner> std::string snapshot(const Owner &owner, const std::vector<List> &regular) {
    std::ostringstream text;
    std::vector<const List *> groups;
    text << owner.attached() << ':';
    const auto binding = [&](const regular_textures::Binding &value) {
        text << std::quoted(value.archive.value_or("<absent>")) << '/';
        if (!value.textures) { text << "null;"; return; }
        auto found = std::find(groups.begin(), groups.end(), value.textures);
        if (found == groups.end()) { groups.push_back(value.textures); found = groups.end() - 1; }
        text << std::distance(groups.begin(), found) << ';';
    };
    for (const auto &language : owner.catalog())
        for (const auto &value : language) binding(value);
    binding(owner.japanese());
    for (const auto *list : groups) {
        text << "list:" << list->entries.size() << ':';
        for (std::size_t i = 0; i < regular.size(); ++i)
            if (list == &regular[i]) text << "regular" << i;
        for (auto attribute : list->attributes) text << '/' << attribute;
        for (const auto &filename : list->filenames)
            text << std::quoted(filename ? *filename : "<absent>");
    }
    return text.str();
}
template<class Function> std::string error(Function function) {
    try { function(); return "completed"; }
    catch (const std::exception &failure) { return failure.what(); }
}
template<class Owner> std::vector<std::string> exercise() {
    std::vector<std::string> rows;
    std::vector<List> regular{native::bind_texture_list(declaration(2, "regular"))};
    Owner owner;
    rows.push_back(snapshot(owner, regular));
    rows.push_back(error([&] { (void)owner.binding(0, 0); }));
    auto input = assets();
    owner.attach(input, regular);
    rows.push_back(snapshot(owner, regular));
    for (std::size_t language = 0; language < owner.catalog().size(); ++language)
        for (std::size_t slot = 0; slot < owner.catalog()[language].size(); ++slot)
            assert(&owner.binding(static_cast<std::int32_t>(language), static_cast<std::uint32_t>(slot)) ==
                   &owner.catalog()[language][slot]);
    assert(owner.binding(0, 2).textures == &regular[0]);
    assert(owner.binding(0, 0).textures == owner.binding(1, 3).textures);
    assert(owner.binding(0, 0).textures != owner.binding(0, 1).textures);
    assert(!owner.binding(0, 0).archive);
    assert(owner.japanese().textures == &regular[0]);
    regular[0].attributes[0] = 987;
    owner.binding(0, 0).textures->attributes[0] = 876;
    rows.push_back(snapshot(owner, regular));
    input.lists.clear();
    rows.push_back(error([&] { owner.attach(input, regular); }));
    rows.push_back(snapshot(owner, regular));
    rows.push_back(error([&] { (void)owner.binding(-1, 0); }));
    rows.push_back(error([&] { (void)owner.binding(5, 0); }));
    rows.push_back(error([&] { (void)owner.binding(0, 30); }));
    for (int failure = 0; failure < 5; ++failure) {
        Owner partial;
        auto broken = assets();
        if (failure == 0) broken.lists[2].regular_texture_list = 99;
        if (failure == 1) broken.lists[2].declaration.texture_count = 3;
        if (failure == 2) broken.lists[1].declaration.attributes.clear();
        if (failure == 3) broken.languages[2][7].texture_list = 99;
        if (failure == 4) broken.japanese_title.texture_list = 99;
        rows.push_back(error([&] { partial.attach(broken, regular); }));
        assert(!partial.attached());
        rows.push_back(snapshot(partial, regular));
        rows.push_back(error([&] { (void)partial.binding(0, 0); }));
        // Retry with the same list shape, preserving the original partial-state contract.
        partial.attach(assets(), regular);
        rows.push_back(snapshot(partial, regular));
    }
    rows.push_back(error([&] { (void)owner.binding(5, 99); }));
    return rows;
}
} // namespace
int main() {
    const auto expected = exercise<sadx::test::PriorMenuTextureBindings>();
#ifdef BASELINE_ONLY
    for (const auto &row : expected) std::cout << row << '\n';
#else
    const auto actual = exercise<sadx::native::MenuTextureBindings>();
    assert(expected == actual);
    std::cout << "Menu texture bindings: alias identity, live lists, ignored payload, partial failures and lookup order pass.\n";
#endif
}
