#include "game/regular_textures.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

extern "C" int sadx_native_regular_textures(const std::uint32_t *in, std::uint32_t *out) {
    using namespace sadx::regular_textures;
    std::fill_n(out, 2048, 0);
    std::array<sadx::texture_lifetime::Memory, 512> memory{};
    std::array<List, 4> lists{};
    const std::array<std::string, 8> names{"", "b32ascii", "other", "b32ascii_j", "CON_REGULAR", "OBJ_REGULAR", "SS_MIZUGI", "CON_REGULAR_E"};
    for (unsigned list = 0; list < 4; ++list) {
        for (unsigned index = 0; index < in[8 + list]; ++index) {
            const auto code = list == 0 ? in[32 + index] : 2;
            Filename filename = code ? std::make_shared<const std::string>(names.at(code)) : nullptr;
            lists[list].filenames.push_back(filename);
            lists[list].attributes.push_back(list == 0 ? in[128 + index] : 0);
            lists[list].entries.push_back(&memory[index % 10 + 1]);
        }
    }
    auto resync = static_cast<std::uint8_t>(in[2]);
    auto language = static_cast<std::int32_t>(in[3]);
    List *common = &lists[3];
    std::array<List *, 32> regular{};
    regular.fill(&lists[3]);
    auto replacement = std::make_shared<const std::string>("b32ascii_j");
    Localizations localizations{{{0, "b32ascii", replacement}}};
    State state{resync, language, common, regular, localizations};
    state.cache.was_loading = in[15] != 0;
    state.cache.directories.push_back(std::make_shared<int>(1));
    state.cache.archives.push_back(std::make_shared<int>(2));
    std::copy_n(in + 256, 300, state.scratch_attributes.begin());
    unsigned calls = 0;
    const auto list_id = [&](const List *list) -> unsigned { return list ? static_cast<unsigned>(list - lists.data()) + 1 : 0; };
    const auto event = [&](unsigned kind, const List *list) {
        const auto offset = 1600 + calls++ * 8;
        out[offset] = kind;
        out[offset + 1] = resync;
        out[offset + 2] = list_id(common);
        out[offset + 3] = list_id(regular[0]);
        out[offset + 4] = list_id(list);
        out[offset + 5] = static_cast<unsigned>(std::count(lists[0].filenames.begin(), lists[0].filenames.end(), replacement));
        out[offset + 6] = state.cache.was_loading;
        out[offset + 7] = static_cast<unsigned>(state.cache.directories.size() + state.cache.archives.size());
        if (in[6] && calls == in[6]) throw 1;
        if (in[7] != 255) resync = static_cast<std::uint8_t>(in[7]);
    };
    Services services{
        [&](std::string_view name) {
            const auto found = std::find(names.begin(), names.end(), name);
            event(static_cast<unsigned>(found - names.begin()) + 10, nullptr);
            ArchiveResult result{static_cast<std::int32_t>(in[5]), {}};
            for (unsigned index = 0; index < in[4]; ++index) result.entries.push_back(&memory[100 + index]);
            return result;
        }, [&](List &list) { event(2, &list); return static_cast<std::int32_t>(in[5]); },
        [&](std::string_view text) { event(text == "SS_MIZUGI\n" ? 3 : 4, nullptr); }};
    Group group{{in[13] & 2 ? std::optional<std::string>{} : names[4], in[13] & 1 ? nullptr : &lists[0]}, {}};
    for (unsigned index = 0; index < in[14]; ++index)
        group.regular.push_back({index % 2 ? names[5] : names[4], &lists[index % 3 + 1]});
    Catalog catalog{&group, &group, nullptr};
    try {
        switch (in[0]) {
        case 0: load_archive(names.at(in[12]), in[13] & 1 ? nullptr : &lists[0], state, services); break;
        case 1: load_no_name(lists[0], state, services); break;
        case 2: out[7] = static_cast<std::uint32_t>(load_individual(lists[0], state, services)); break;
        case 3: load_regular(in[1], catalog, state, services); break;
        case 4: check_cache(state); break;
        }
    } catch (int) { out[0] = 1; }
    catch (const Error &error) { out[0] = 2 + static_cast<unsigned>(error.boundary()); }
    out[1] = resync;
    out[2] = state.cache.was_loading;
    out[3] = static_cast<unsigned>(state.cache.directories.size() + state.cache.archives.size());
    out[4] = list_id(common);
    out[5] = calls;
    for (unsigned index = 0; index < 32; ++index) out[16 + index] = list_id(regular[index]);
    for (unsigned list = 0; list < 4; ++list) {
        out[48 + list] = static_cast<unsigned>(lists[list].entries.size());
        for (unsigned index = 0; index < lists[list].entries.size(); ++index) {
            const auto offset = 64 + list * 300 + index * 3;
            const auto filename = index < lists[list].filenames.size() ? lists[list].filenames[index] : nullptr;
            out[offset] = filename == replacement ? 9000 : filename ? index + 1 : 0;
            out[offset + 1] = index < lists[list].attributes.size() ? lists[list].attributes[index] : 0;
            out[offset + 2] = lists[list].entries[index] ? static_cast<unsigned>(lists[list].entries[index] - memory.data()) : 0;
        }
    }
    return 1;
}
