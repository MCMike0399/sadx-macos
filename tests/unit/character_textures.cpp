#include "game/character_textures.hpp"
#include <array>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::vector<std::string> *amy_events = nullptr;
bool amy_fails = false;
} // namespace
namespace sadx::regular_textures {
void load_egg_robo_textures(State &, Services &) {
    amy_events->push_back("amy-extra");
    if (amy_fails)
        throw std::runtime_error("unrecovered LoadEggRoboTextures");
}
} // namespace sadx::regular_textures

int main() {
    using namespace sadx::regular_textures;
    std::uint8_t resync = 0;
    std::int32_t language = 1;
    List *common = nullptr;
    std::array<List *, 32> regular{};
    Localizations localizations{};
    State state{resync, language, common, regular, localizations, {}, {}};
    std::array<List, 3> lists{};
    for (auto &list : lists)
        list.entries.resize(1);
    std::array<CharacterBinding, 4> entries{{{std::string_view("SONIC"), &lists[0]},
                                             {std::nullopt, &lists[1]},
                                             {std::string_view("E102"), &lists[2]},
                                             {std::string_view("ignored"), nullptr}}};
    std::array<CharacterTable, 9> tables{};
    tables.fill({entries.data(), entries.size()});
    std::array<std::int8_t, 9> loaded{};
    CharacterCatalog catalog{tables.data(), loaded.data(), loaded.size()};
    std::vector<std::string> events;
    amy_events = &events;
    Services services{[&](std::string_view name) {
                          events.emplace_back(name);
                          return ArchiveResult{0, {nullptr}};
                      },
                      [&](List &list) {
                          assert(&list == &lists[1]);
                          events.push_back("nameless");
                          return 0;
                      },
                      [&](std::string_view) { assert(false); }};
    load_character(0, catalog, state, services);
    assert((events == std::vector<std::string>{"SONIC", "nameless", "E102"}));
    assert(loaded[0] == 1 && resync == 1);
    events.clear();
    load_character(0, catalog, state, services);
    assert(events.empty() && loaded[0] == 2);

    for (unsigned value = 0; value < 256; ++value) {
        loaded[1] = value < 128 ? static_cast<std::int8_t>(value)
                                : static_cast<std::int8_t>(static_cast<int>(value) - 256);
        events.clear();
        load_character(1, catalog, state, services);
        assert(static_cast<std::uint8_t>(loaded[1]) == static_cast<std::uint8_t>(value + 1));
        assert(events.size() == (value == 0 ? 3 : 0));
        tables[1].entries = nullptr;
        const auto before = loaded[1];
        load_character(1, catalog, state, services);
        assert(loaded[1] == before);
        tables[1].entries = entries.data();
    }
    events.clear();
    load_character(5, catalog, state, services);
    assert((events == std::vector<std::string>{"SONIC", "nameless", "E102", "amy-extra"}));
    events.clear();
    load_character(5, catalog, state, services);
    assert(events.empty() && loaded[5] == 2);
    loaded[5] = 0;
    tables[5].count = 0;
    load_character(5, catalog, state, services);
    assert((events == std::vector<std::string>{"amy-extra"}));
    assert(loaded[5] == 1);

    // A missing Amy implementation fails before the wrapper's counter store.
    events.clear();
    loaded[5] = 0;
    amy_fails = true;
    try {
        load_character(5, catalog, state, services);
        assert(false);
    } catch (const std::runtime_error &) {
    }
    assert(loaded[5] == 0);
    amy_fails = false;

    // Real maintained lower loader callbacks mutate the borrowed owners.
    events.clear();
    loaded[0] = 0;
    services.load_archive = [&](std::string_view name) {
        events.emplace_back(name);
        if (name == "SONIC") {
            loaded[0] = 126;
            entries[1].textures = nullptr;
            tables[0] = {nullptr, 0};
        }
        return ArchiveResult{0, {nullptr}};
    };
    load_character(0, catalog, state, services);
    assert((events == std::vector<std::string>{"SONIC"}));
    assert(loaded[0] == 127);

    // Rebinding the directory must not replace the already captured table.
    entries[1].textures = &lists[1];
    tables[0] = {entries.data(), entries.size()};
    loaded[0] = 0;
    events.clear();
    services.load_archive = [&](std::string_view name) {
        events.emplace_back(name);
        tables[0] = {nullptr, 0};
        return ArchiveResult{0, {nullptr}};
    };
    load_character(0, catalog, state, services);
    assert((events == std::vector<std::string>{"SONIC", "nameless", "E102"}));
    assert(loaded[0] == 1);

    tables[0] = {entries.data(), entries.size()};
    loaded[0] = 0;
    services.load_archive = [](std::string_view) -> ArchiveResult {
        throw std::runtime_error("lower load failure");
    };
    try {
        load_character(0, catalog, state, services);
        assert(false);
    } catch (const std::runtime_error &) {
    }
    assert(loaded[0] == 0);
    try {
        load_character(9, catalog, state, services);
        assert(false);
    } catch (const std::out_of_range &) {
    }
    std::cout << "character texture wrapper: maintained lower loaders, 256 byte values, live "
                 "owners pass\n";
}
