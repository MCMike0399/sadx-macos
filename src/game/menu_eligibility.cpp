#include "game/menu_eligibility.hpp"

namespace sadx::menu_eligibility {
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
    auto enabled = std::int32_t(progress.story_flags[89] != 0);
    if (enabled == 0 && emblem_count(progress.save) >= 130) {
        progress.story_flags[89] = 1;
        enabled = 1;
    }
    return enabled;
}
std::int32_t character_enabled(std::uint32_t character, Progress progress) {
    if (progress.unlock_override != 0)
        return 1;
    constexpr std::array<unsigned, 7> flags{65, 66, 67, 68, 70, 69, 71};
    if (character >= flags.size())
        throw UnknownCharacter(character);
    if (character == 6 && progress.tutorial_variant == 1)
        return metal_sonic_enabled(progress);
    // SeqCheckFlag clears EAX and SETNZ writes AL, so valid returns are
    // exactly0/1.
    return progress.story_flags[flags[character]] != 0 ? 1 : 0;
}
} // namespace sadx::menu_eligibility
