#include "game/regular_textures.hpp"
#include <utility>

namespace sadx::regular_textures {
void check_cache(State &state) {
    if (state.resync != 0)
        return;
    if (state.cache.was_loading) {
        state.cache.directories.clear();
        state.cache.archives.clear();
    }
    state.cache.was_loading = false;
}
void load_archive(std::string_view name, List *list, State &state, Services &services) {
    if (name == "SS_MIZUGI")
        services.diagnostic("SS_MIZUGI\n");
    if (list == nullptr) {
        services.diagnostic("\\SADXPC\\sonic\\src\\texture.c 2675 : ptlo == 0\n");
        return;
    }
    if (list->entries.empty())
        return;
    check_cache(state);
    state.resync = 1;
    auto archive = services.load_archive(name);
    if (archive.status == -1)
        return;
    if (archive.entries.size() > state.scratch_attributes.size())
        throw Error(Boundary::ScratchCapacity, "PVM exceeds the source temporary texture-name buffer");
    if (archive.entries.size() > list->entries.size()) {
        if (list->filenames.size() != list->entries.size())
            throw Error(Boundary::FilenameStorage, "PVM expansion requires the original filename members");
        list->filenames.resize(archive.entries.size());
        list->entries.resize(archive.entries.size());
    }
    list->attributes.resize(list->entries.size());
    texture_lifetime::Memory *memory = nullptr;
    std::uint32_t attributes = 0;
    for (std::size_t index = 0; index < list->entries.size(); ++index) {
        if (index < archive.entries.size()) {
            memory = archive.entries[index];
            attributes = state.scratch_attributes[index];
        }
        list->attributes[index] = attributes;
        list->entries[index] = memory;
    }
}
std::int32_t load_individual(List &list, State &state, Services &services) {
    check_cache(state);
    state.resync = 1;
    return services.load_individual(list);
}
void load_no_name(List &list, State &state, Services &services) {
    struct ChangedName {
        std::size_t index;
        Filename original;
    };
    std::vector<ChangedName> changed;
    for (std::size_t index = 0; index < list.entries.size() && changed.size() < 63; ++index) {
        for (const auto &rule : state.localizations) {
            if (rule.language != state.language)
                continue;
            if (index >= list.filenames.size() || list.filenames[index] == nullptr)
                throw Error(Boundary::Filename, "LoadNoNamePVM localization requires a text filename");
            if (*list.filenames[index] == rule.source) {
                changed.push_back({index, list.filenames[index]});
                list.filenames[index] = rule.replacement;
                break;
            }
        }
    }
    load_individual(list, state, services);
    // Restoration happens only after the loader returns, and in reverse order.
    while (!changed.empty()) {
        const auto change = std::move(changed.back());
        changed.pop_back();
        list.filenames[change.index] = change.original;
    }
    state.resync = 1;
}
namespace {
// NOTSA: the terminating pointer after a full 32-entry array aliases CommonTextures.
void store_regular(std::size_t index, List *list, State &state) {
    if (index < state.regular.size()) {
        state.regular[index] = list;
    } else if (index == state.regular.size()) {
        state.common = list;
    } else {
        throw Error(Boundary::RegularCapacity, "regular texture registration exceeds its source owners");
    }
}
// NOTSA: share the named/anonymous descriptor dispatch used by the two loops.
void load_binding(const Binding &binding, bool regular, State &state, Services &services) {
    if (!binding.archive) {
        load_no_name(*binding.textures, state, services);
    } else if (regular && *binding.archive == "CON_REGULAR" && state.language != 0) {
        load_archive("CON_REGULAR_E", binding.textures, state, services);
    } else {
        load_archive(*binding.archive, binding.textures, state, services);
    }
}
} // namespace
void load_regular(std::uint32_t index, const Catalog &catalog, State &state, Services &services) {
    if (index >= catalog.size())
        throw Error(Boundary::GroupIndex, "regular texture group is outside the imported table");
    const auto *group = catalog[index];
    if (group == nullptr) {
        state.common = nullptr;
        return;
    }
    state.common = group->common.textures;
    if (state.common != nullptr)
        load_binding(group->common, false, state, services);
    std::size_t slot = 0;
    for (const auto &binding : group->regular) {
        if (binding.textures == nullptr)
            break;
        load_binding(binding, true, state, services);
        store_regular(slot++, binding.textures, state);
    }
    store_regular(slot, nullptr, state);
}
} // namespace sadx::regular_textures
