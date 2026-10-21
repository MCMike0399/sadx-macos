#include "game/character_select_menu.hpp"
#include <algorithm>
extern "C" std::int32_t sadx_native_character_select_availability(const std::uint8_t *input, unsigned kind,
                                                          std::int32_t character,
                                                          const std::uint8_t *stages,
                                                          const std::uint16_t *characters, std::uint8_t *output) {
    sadx::save::Data save{};
    save.options = input[0];
    std::copy_n(input + 1, 17, save.emblems.begin());
    std::array<std::uint8_t, 512> flags{};
    std::copy_n(input + 18, 512, flags.begin());
    const std::uint8_t override = input[530], tutorial = input[531];
    sadx::menu_eligibility::Progress progress{save, flags, override, tutorial};
    std::array<std::uint8_t, 344> completed{};
    std::copy_n(input + 532, 344, completed.begin());
    sadx::title_menu::StageSelectionData catalog{};
    auto catalog_bytes = stages;
    for (unsigned group = 0; group < 2; ++group)
        for (unsigned i = 0; i < 6; ++i) {
            auto &rows = group == 0 ? catalog.trial_stages[i] : catalog.subgame_stages[i];
            const auto count = *catalog_bytes++;
            for (unsigned n = 0; n < count; ++n) {
                rows.push_back({catalog_bytes[0], catalog_bytes[1]});
                catalog_bytes += 2;
            }
        }
    for (unsigned i = 0; i < 7; ++i)
        catalog.character_ids[i] = characters[i];
    const std::uint8_t subgame = input[876];
    const std::array<std::uint32_t, 7> clear_flags{72,73,74,75,77,76,78};
    sadx::character_select_menu::Availability availability{progress,{completed,catalog},subgame,clear_flags};
    const auto result = kind == 0 ? sadx::character_select_menu::character_count(progress)
        : kind == 1 ? sadx::character_select_menu::character_cleared(character,availability)
        : sadx::character_select_menu::trial_character_enabled(character,availability);
    std::copy(flags.begin(), flags.end(), output);
    return result;
}
