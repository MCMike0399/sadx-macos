#include "game/character_textures.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

namespace {
using namespace sadx::regular_textures;
struct Fixture {
    const std::uint32_t *in;
    std::uint32_t *out;
    std::array<List, 8> lists{};
    std::array<CharacterBinding, 8> entries{};
    std::array<CharacterBinding, 8> alternate{};
    std::array<CharacterTable, 9> tables{};
    std::array<std::int8_t, 9> loaded{};
    std::array<std::string, 5> names{"", "SONIC", "AMY", "E102", "EXTRA"};
    unsigned calls = 0, entry = 0;
    Fixture(const std::uint32_t *values, std::uint32_t *results) : in(values), out(results) {}
    void receipt(unsigned kind, std::string_view name, List *list) {
        const auto name_id =
            static_cast<unsigned>(std::find(names.begin(), names.end(), name) - names.begin());
        const auto list_id = list ? static_cast<unsigned>(list - lists.data()) + 1 : 0;
        const auto at = 32 + calls++ * 4;
        out[at] = kind;
        out[at + 1] = name_id;
        out[at + 2] = list_id;
        out[at + 3] = static_cast<std::uint8_t>(loaded[in[0]]);
        if (calls == in[4]) {
            if (in[5] & 1)
                loaded[in[0]] = static_cast<std::int8_t>(in[6]);
            if ((in[5] & 2) && entry + 1 < entries.size())
                entries[entry + 1].textures = in[7] ? &lists[in[7] - 1] : nullptr;
            if ((in[5] & 4) && entry + 1 < entries.size())
                entries[entry + 1].archive =
                    in[8] ? std::optional<std::string_view>(names[in[8]]) : std::nullopt;
            if (in[5] & 8)
                tables[in[0]] = {alternate.data(), alternate.size()};
            if (in[5] & 16)
                entries[entry].textures = nullptr;
            if (in[5] & 32)
                throw 1;
        }
        if (kind != 3)
            ++entry;
    }
};
thread_local Fixture *active = nullptr;
} // namespace

// Ordered lower-callee receipts only: these do not implement the lower algorithms.
namespace sadx::regular_textures {
void load_archive(std::string_view name, List *list, State &, Services &) {
    active->receipt(1, name, list);
}
void load_no_name(List &list, State &, Services &) { active->receipt(2, "", &list); }
void load_egg_robo_textures(State &, Services &) { active->receipt(3, "", nullptr); }
} // namespace sadx::regular_textures

extern "C" int sadx_native_character_textures(const std::uint32_t *in, std::uint32_t *out) {
    using namespace sadx::regular_textures;
    std::fill_n(out, 512, 0);
    Fixture fixture{in, out};
    for (unsigned i = 0; i < fixture.entries.size(); ++i) {
        fixture.entries[i] = {in[16 + i * 2]
                                  ? std::optional<std::string_view>(fixture.names[in[16 + i * 2]])
                                  : std::nullopt,
                              in[17 + i * 2] ? &fixture.lists[in[17 + i * 2] - 1] : nullptr};
        fixture.alternate[i] = {fixture.names[4], nullptr};
    }
    fixture.tables.fill({fixture.entries.data(), in[9]});
    fixture.loaded.fill(37);
    fixture.loaded[in[0]] = static_cast<std::int8_t>(in[1]);
    if (in[2])
        fixture.tables[in[0]].entries = nullptr;
    std::uint8_t resync = 0;
    std::int32_t language = 1;
    List *common = nullptr;
    std::array<List *, 32> regular{};
    Localizations localizations{};
    State state{resync, language, common, regular, localizations, {}, {}};
    Services services{};
    active = &fixture;
    try {
        for (unsigned i = 0; i < in[10]; ++i) {
            fixture.entry = 0;
            load_character(in[0],
                           {fixture.tables.data(), fixture.loaded.data(), fixture.tables.size()},
                           state, services);
        }
    } catch (int) {
        out[0] = 1;
    }
    active = nullptr;
    out[1] = fixture.calls;
    for (unsigned i = 0; i < fixture.loaded.size(); ++i)
        out[8 + i] = static_cast<std::uint8_t>(fixture.loaded[i]);
    out[17] = fixture.tables[in[0]].entries == fixture.alternate.data();
    for (unsigned i = 0; i < fixture.entries.size(); ++i) {
        const auto &binding = fixture.entries[i];
        out[400 + i * 2] =
            binding.archive
                ? static_cast<unsigned>(
                      std::find(fixture.names.begin(), fixture.names.end(), *binding.archive) -
                      fixture.names.begin())
                : 0;
        out[401 + i * 2] = binding.textures
                               ? static_cast<unsigned>(binding.textures - fixture.lists.data()) + 1
                               : 0;
    }
    return 1;
}
