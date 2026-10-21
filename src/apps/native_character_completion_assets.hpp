#pragma once
#include "apps/native_assets.hpp"
#include "game/character_completion.hpp"

namespace sadx::native::app {
// NOTSA: source-authored progress rows; the runtime continues to borrow live flags.
inline character_completion::Catalog load_character_completion_assets(
    const std::filesystem::path &directory) {
    const auto source = read_metadata(directory / "character-completion.json");
    character_completion::Catalog result{};
    const auto catalogs = array(source[@"catalogs"], 7, "character completion catalogs");
    constexpr std::array<std::size_t, 7> counts{19, 14, 10, 10, 11, 10, 1};
    for (std::size_t index = 0; index < result.size(); ++index) {
        const auto catalog = dictionary(catalogs[index], "character completion catalog");
        (void)integer(catalog[@"selection"], index, index, "character completion selection");
        for (id entry in array(catalog[@"rows"], counts[index], "character completion rows")) {
            const auto row = dictionary(entry, "character completion row");
            const auto kind = static_cast<std::int32_t>(
                integer(row[@"source"], INT32_MIN, INT32_MAX, "completion flag source"));
            const auto flag = static_cast<std::int32_t>(integer(
                row[@"flag"], kind == 0 || kind == 1 ? 0 : INT32_MIN,
                kind == 0 || kind == 1 ? 511 : INT32_MAX, "completion flag index"));
            result[index].push_back({static_cast<std::int32_t>(
                                        integer(row[@"value"], INT32_MIN, INT32_MAX,
                                                "completion authored value")),
                                    static_cast<character_completion::FlagSource>(kind), flag});
        }
    }
    return result;
}
} // namespace sadx::native::app
