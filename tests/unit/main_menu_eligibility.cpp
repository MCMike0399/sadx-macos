#include "game/main_menu_eligibility.hpp"
#include <cassert>
int main() {
    sadx::save::Data save{};
    std::array<std::uint8_t, 512> flags{};
    std::array<std::uint8_t, 344> completed{};
    std::uint8_t override = 0, tutorial = 0;
    sadx::title_menu::StageSelectionData catalog{};
    for (unsigned i = 0; i < 6; ++i) {
        catalog.character_ids[i] = i;
        catalog.trial_stages[i] = {{1, 0}, {2, 0}};
    }
    sadx::menu_eligibility::Progress progress{save, flags, override, tutorial};
    sadx::menu_eligibility::Completion completion{completed, catalog};
    assert(sadx::menu_eligibility::trial_enabled(progress, completion) == 0);
    flags[65] = 255;
    assert(sadx::menu_eligibility::trial_enabled(progress, completion) == 0);
    completed[2] = 255;
    assert(sadx::menu_eligibility::trial_enabled(progress, completion) == 1);
    flags[65] = 0;
    assert(sadx::menu_eligibility::trial_enabled(progress, completion) == 0);
    flags[77] = 255;
    assert(sadx::menu_eligibility::mission_enabled(progress) == 1);
    flags[77] = 0;
    assert(sadx::menu_eligibility::mission_enabled(progress) == 0);
    override = 255;
    assert(sadx::menu_eligibility::mission_enabled(progress) == 1);
    save.options = 0;
    assert(sadx::menu_eligibility::voice_language(save, 4) == 1 &&
           sadx::menu_eligibility::text_language(save, 4) == 4);
    save.options = 0x54;
    assert(sadx::menu_eligibility::voice_language(save, 4) == 0 &&
           sadx::menu_eligibility::text_language(save, 4) == 4);

    override = 0;
    completed.fill(0);
    completed[1] = 1;
    completed[43 + 2] = 255;
    assert(sadx::menu_eligibility::trial_stage_count(0, progress, completion, 1) == 2);
    assert(sadx::menu_eligibility::trial_stage_count(0, progress, completion, 0) == 0);
    std::array<std::uint8_t, 60> missions{};
    for (unsigned i = 0; i < 19; ++i)
        missions[i] = 128;
    missions[19] = 1;
    assert(sadx::menu_eligibility::completed_mission_count(missions) == 19);
    assert(sadx::menu_eligibility::optional_entry_enabled(progress, missions) == 0);
    missions[19] = 128;
    assert(sadx::menu_eligibility::optional_entry_enabled(progress, missions) == 1);
}
