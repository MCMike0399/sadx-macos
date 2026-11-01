#pragma once
#include "game/save_data.hpp"
#include "game/story_flags.hpp"
#include <array>
#include <cstdint>
#include <stdexcept>

namespace sadx::menu_eligibility {
// NOTSA: names the roster and the two selection policies observed by the predicates.
inline constexpr std::uint32_t kRegularCharacterCount = 6;
inline constexpr std::uint32_t kExtraCharacterSlot = kRegularCharacterCount;
inline constexpr std::uint8_t kMetalSelectionVariant = 1;
inline constexpr std::uint8_t kRegularCharactersOnlyVariant = 2;
// NOTSA: borrows the live persistent story flags; these are separate from
// cutscene event work.
struct Progress {
    const save::Data &save;
    std::array<std::uint8_t, 512> &story_flags;
    const std::uint8_t &unlock_override, &selection_variant;
};
class UnknownCharacter : public std::runtime_error {
  public:
    explicit UnknownCharacter(std::uint32_t character)
        : std::runtime_error("character selection is outside the supported roster"),
          character_(character) {}
    std::uint32_t character() const { return character_; }

  private:
    std::uint32_t character_;
};
// FUNCTION: SONIC 0x4b3f90 GetEmblemCount
std::int32_t emblem_count(const save::Data &);
// FUNCTION: SONIC 0x506490 IsMetalSonicUnlocked
std::int32_t metal_sonic_enabled(Progress);
// FUNCTION: SONIC 0x506630 GetEachCharEnable
std::int32_t character_enabled(std::uint32_t character, Progress);
} // namespace sadx::menu_eligibility
