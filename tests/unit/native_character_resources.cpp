#include "character_resource_baseline.hpp"
#ifndef CHARACTER_RESOURCES_BASELINE_ONLY
#include "runtime/native_character_resources.hpp"
#endif
#include <cassert>
#include <iostream>
#include <sstream>
#include <type_traits>

namespace {
using namespace sadx;
native::CharacterTextureAssets assets() {
    native::CharacterTextureAssets result{};
    for (std::size_t character = 0; character < result.size(); ++character)
        for (std::size_t list = 0; list < 2; ++list)
            result[character].push_back({"archive-" + std::to_string(character) + "-" + std::to_string(list),
                                         "resource-" + std::to_string(character), character + list + 1});
    return result;
}
template <class Owner> std::string snapshot(Owner &owner) {
    std::ostringstream out;
    for (std::size_t character = 0; character < owner.assets().size(); ++character) {
        out << "C" << character << ':' << int(owner.loaded()[character]);
        for (const auto &asset : owner.assets()[character])
            out << '/' << asset.archive << '/' << asset.resource_name << '/' << asset.texture_count;
        for (const auto &list : owner.lists()[character]) {
            out << " L" << list.entries.size() << ':' << list.filenames.size() << ':' << list.attributes.size();
            for (const auto *entry : list.entries) out << (entry ? '1' : '0');
        }
        const auto *binding = owner.bindings()[character];
        out << " B" << bool(binding);
        if (binding) {
            assert(binding->lists.size() == owner.lists()[character].size());
            for (std::size_t list = 0; list < binding->lists.size(); ++list) {
                assert(binding->lists[list] == &owner.lists()[character][list]);
                out << '/' << list;
            }
        }
        out << ';';
    }
    return out.str();
}
template <class Owner> std::vector<std::string> exercise() {
    std::vector<std::string> observations;
    for (int failure = 0; failure < 7; ++failure) {
        auto declarations = assets();
        if (failure == 1) declarations[0].clear();
        if (failure == 2) declarations[8].clear();
        if (failure == 3) declarations[0][0].texture_count = 0;
        if (failure == 4) declarations[0][1].texture_count = 0;
        if (failure == 5) declarations[4][0].texture_count = 0;
        if (failure == 6) declarations[8][1].texture_count = 0;
        Owner owner(declarations);
        observations.push_back(snapshot(owner)); // Binding stays at the Session body boundary.
        for (const auto *binding : owner.bindings()) assert(!binding);
        for (const auto &lists : owner.lists()) assert(lists.empty());
        auto &counts = owner.loaded();
        assert(&counts == &static_cast<const Owner &>(owner).loaded());
        try {
            owner.initialize();
            assert(failure == 0);
            observations.push_back("initialized");
        } catch (const std::runtime_error &error) {
            assert(failure != 0);
            observations.push_back(error.what());
        }
        observations.push_back(snapshot(owner));
        if (failure) continue;
        declarations[0][0].archive = "input changed after copy";
        assert(owner.assets()[0][0].archive == "archive-0-0");
        native::CharacterTextureViews views(owner.assets(), owner.lists());
        auto catalog = views.catalog(counts);
        assert(catalog.loaded == counts.data() && catalog.count == 9);
        for (std::size_t character = 0; character < 9; ++character) {
            assert(catalog.tables[character].entries[0].textures == &owner.lists()[character][0]);
            assert(catalog.tables[character].entries[0].archive->data() == owner.assets()[character][0].archive.data());
        }
        counts[0] = -128; counts[1] = 127; counts[8] = -1;
        assert(catalog.loaded[0] == -128 && catalog.loaded[1] == 127 && catalog.loaded[8] == -1);
        observations.push_back(snapshot(owner));
        sprite::TextureList *common = nullptr;
        std::array<sprite::TextureList *, 32> regular{};
        cleanup::TextureState cleanup_state{common, regular, owner.bindings(), counts};
        texture_lifetime::Memory memory{};
        owner.lists()[0][0].entries[0] = &memory;
        counts[0] = 1;
        unsigned calls = 0;
        cleanup::unload_character_textures(0, cleanup_state, [&](auto &list) {
            ++calls; assert(&list == &owner.lists()[0][0]);
            counts[0] = 3; // Final decrement must read the callback's live value.
        });
        assert(calls == 1 && counts[0] == 2 && catalog.loaded[0] == 2);
        observations.push_back(snapshot(owner));
        counts[0] = 1;
        try {
            cleanup::unload_character_textures(0, cleanup_state, [&](auto &) {
                counts[0] = 4;
                throw std::runtime_error("release failed");
            });
            assert(false);
        } catch (const std::runtime_error &error) {
            observations.push_back(error.what());
        }
        assert(counts[0] == 4);
        observations.push_back(snapshot(owner));
    }
    return observations;
}
} // namespace
int main() {
    const auto baseline = exercise<sadx::test::FrozenCharacterResources>();
#ifndef CHARACTER_RESOURCES_BASELINE_ONLY
    static_assert(!std::is_copy_constructible_v<sadx::native::CharacterResources>);
    static_assert(!std::is_move_constructible_v<sadx::native::CharacterResources>);
    const auto current = exercise<sadx::native::CharacterResources>();
    assert(current == baseline);
#endif
    for (const auto &row : baseline) std::cout << row << '\n';
}
