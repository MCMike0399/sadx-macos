#pragma once
#include "game/menu_eligibility.hpp"
#include "game/title_menu.hpp"
namespace sadx::menu_eligibility {
// NOTSA: borrows canonical persistent completion and source-backed stage
// catalogs.
struct Completion {
    const std::array<std::uint8_t, 344> &completed_stages;
    const title_menu::StageSelectionData &catalog;
};
// FUNCTION: SONIC 0x506410 AvaGetMissionEnable
std::int32_t mission_enabled(Progress);
// FUNCTION: SONIC 0x506780 AvaGetTrialEnable
std::int32_t trial_enabled(Progress, Completion);
// FUNCTION: SONIC 0x506840 GetVoiceLanguage
std::uint8_t voice_language(const save::Data &, std::int32_t language);
// FUNCTION: SONIC 0x5068a0 GetTextLanguage
std::int8_t text_language(const save::Data &, std::int32_t language);

// FUNCTION: SONIC 0x4b51a0 CountCompletedMissions
std::uint8_t completed_mission_count(const std::array<std::uint8_t, 60> &missions);
// FUNCTION: SONIC 0x506460 OptionalEntryEnabled
std::int32_t optional_entry_enabled(Progress, const std::array<std::uint8_t, 60> &missions);
// FUNCTION: SONIC 0x5060f0 GetEnableTrialActNum
std::uint8_t trial_stage_count(unsigned character, Progress, Completion,
                               std::uint8_t current_character);
// FUNCTION: SONIC 0x5061a0 GetEnableMiniGameNum
std::uint8_t minigame_count(unsigned character, Progress, Completion);
// FUNCTION: SONIC 0x5061f0 GetEnableTrialActNumAll
std::uint8_t total_trial_stage_count(Progress, Completion, std::uint8_t current_character);
// FUNCTION: SONIC 0x506210 GetEnableMiniGameNumAll
std::uint8_t total_minigame_count(Progress, Completion);
} // namespace sadx::menu_eligibility
