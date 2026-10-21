#include "game/main_menu_eligibility.hpp"
#include <algorithm>
extern "C" std::int32_t sadx_native_main_menu_eligibility(const std::uint8_t *input, unsigned kind,
                                                          std::int32_t language,
                                                          const std::uint8_t *stages,
                                                          const std::uint16_t *characters) {
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
    std::array<std::uint8_t, 60> missions{};
    std::copy_n(input + 877, 60, missions.begin());
    switch (kind) {
    case 0:
        return sadx::menu_eligibility::mission_enabled(progress);
    case 1:
        return sadx::menu_eligibility::trial_enabled(progress, {completed, catalog});
    case 2:
        return sadx::menu_eligibility::voice_language(save, language);
    case 3:
        return sadx::menu_eligibility::text_language(save, language);
    case 4:
        return sadx::menu_eligibility::optional_entry_enabled(progress, missions);
    case 5:
        return sadx::menu_eligibility::total_trial_stage_count(progress, {completed, catalog},
                                                               input[876]);
    case 6:
        return sadx::menu_eligibility::total_minigame_count(progress, {completed, catalog});
    default:
        return sadx::menu_eligibility::completed_mission_count(missions);
    }
}
