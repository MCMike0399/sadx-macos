#include "game/menu_eligibility.hpp"

namespace sadx::menu_eligibility {
namespace {
constexpr std::int32_t kMetalUnlockEmblems = 130;
}
std::int32_t emblem_count(const save::Data &save) {
    std::int32_t result = 0;
    for (auto byte : save.emblems) {
        while (byte != 0) {
            if ((byte & 1U) != 0)
                ++result;
            byte >>= 1;
        }
    }
    return result;
}
std::int32_t metal_sonic_enabled(Progress progress) {
    if (progress.unlock_override != 0)
        return 1;
    auto enabled = std::int32_t(progress.story_flags[story_flags::kMetalSonicPlayable] != 0);
    if (enabled == 0 && emblem_count(progress.save) >= kMetalUnlockEmblems) {
        progress.story_flags[story_flags::kMetalSonicPlayable] = 1;
        enabled = 1;
    }
    return enabled;
}
std::int32_t character_enabled(std::uint32_t character, Progress progress) {
    if (progress.unlock_override != 0)
        return 1;
    constexpr std::array<std::size_t, 7> flags{
        story_flags::kSonicPlayable, story_flags::kTailsPlayable,
        story_flags::kKnucklesPlayable, story_flags::kAmyPlayable,
        story_flags::kBigPlayable, story_flags::kGammaPlayable,
        story_flags::kSuperSonicPlayable};
    if (character >= flags.size())
        throw UnknownCharacter(character);
    if (character == kExtraCharacterSlot && progress.selection_variant == kMetalSelectionVariant)
        return metal_sonic_enabled(progress);
    // Story flag bytes are tested for nonzero; valid results are exactly 0 or 1.
    return progress.story_flags[flags[character]] != 0 ? 1 : 0;
}
} // namespace sadx::menu_eligibility
