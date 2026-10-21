#include "game/main_menu_eligibility.hpp"
namespace sadx::menu_eligibility {
std::int32_t mission_enabled(Progress progress) {
    if (progress.unlock_override != 0)
        return 1;
    constexpr std::array<unsigned, 6> clear_flags{72, 73, 74, 75, 77, 76};
    for (auto flag : clear_flags)
        if (progress.story_flags[flag] != 0)
            return 1;
    return 0;
}
std::int32_t trial_enabled(Progress progress, Completion completion) {
    if (progress.unlock_override != 0)
        return 1;
    for (unsigned character = 0; character < 6; ++character) {
        if (character_enabled(character, progress) == 0)
            continue;
        const auto stage = completion.catalog.trial_stages[character].at(1).stage;
        const auto source_character = completion.catalog.character_ids[character];
        if (completion.completed_stages.at(source_character * 43 + stage) != 0)
            return 1;
    }
    return 0;
}
std::uint8_t voice_language(const save::Data &save, std::int32_t language) {
    const auto voice = (save.options >> 2) & 3;
    return voice == 0 ? language != 0 : voice != 1;
}
std::int8_t text_language(const save::Data &save, std::int32_t language) {
    const auto text = (save.options >> 4) & 7;
    return static_cast<std::int8_t>(text >= 1 && text <= 5 ? text - 1 : language);
}

std::uint8_t completed_mission_count(const std::array<std::uint8_t, 60> &missions) {
    std::uint8_t count = 0;
    for (auto mission : missions)
        if ((mission & 128) != 0)
            ++count;
    return count;
}
std::int32_t optional_entry_enabled(Progress progress,
                                    const std::array<std::uint8_t, 60> &missions) {
    return progress.unlock_override != 0 || emblem_count(progress.save) >= 20 ||
           completed_mission_count(missions) >= 20;
}
std::uint8_t trial_stage_count(unsigned character, Progress progress, Completion completion,
                               std::uint8_t current_character) {
    const auto &stages = completion.catalog.trial_stages.at(character);
    if (progress.unlock_override != 0)
        return static_cast<std::uint8_t>(stages.size());
    const auto source_character = completion.catalog.character_ids.at(character);
    // NOTSA: names repeated reads from the canonical stage completion table.
    const auto cleared = [&](unsigned row, std::uint8_t stage) {
        return completion.completed_stages.at(row * 43 + stage) != 0;
    };
    if (!cleared(source_character, stages.at(0).stage))
        return 0;
    std::uint8_t count = 0;
    for (unsigned i = 1; i < stages.size(); ++i)
        if (cleared(source_character, stages[i].stage))
            ++count;
    // Source quirk: the final stage is tested against the current menu character.
    if (cleared(completion.catalog.character_ids.at(current_character), stages.back().stage))
        return static_cast<std::uint8_t>(stages.size());
    return count;
}
std::uint8_t minigame_count(unsigned character, Progress progress, Completion completion) {
    const auto &stages = completion.catalog.subgame_stages.at(character);
    const auto source_character = completion.catalog.character_ids.at(character);
    std::uint8_t count = 0;
    for (const auto &stage : stages)
        if (completion.completed_stages.at(source_character * 43 + stage.stage) != 0 ||
            progress.unlock_override != 0)
            ++count;
    return count;
}
std::uint8_t total_trial_stage_count(Progress progress, Completion completion,
                                     std::uint8_t current_character) {
    std::uint8_t count = 0;
    for (unsigned character = 0; character < 6; ++character)
        count += trial_stage_count(character, progress, completion, current_character);
    return count;
}
std::uint8_t total_minigame_count(Progress progress, Completion completion) {
    std::uint8_t count = 0;
    for (unsigned character = 0; character < 6; ++character)
        count += minigame_count(character, progress, completion);
    return count;
}
} // namespace sadx::menu_eligibility
