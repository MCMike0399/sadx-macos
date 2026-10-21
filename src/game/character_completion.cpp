#include "game/character_completion.hpp"

namespace sadx::character_completion {
namespace {
// NOTSA: checked borrowed views of the existing fixed byte-read contracts. The
// legacy SeqCheckFlag seam keeps its own source annotation and owner.
std::uint8_t read_flag(const std::array<std::uint8_t, 512> &flags, std::int32_t index,
                       Boundary boundary) {
    if (index < 0 || static_cast<std::size_t>(index) >= flags.size())
        throw Error(boundary, index);
    return flags[static_cast<std::size_t>(index)];
}
// NOTSA: source helpers are pure flag reads; this view retains their distinct
// nonzero story predicate and signed cutscene return without duplicate storage.
bool completed(const Row &row, Flags flags) {
    if (row.source == FlagSource::Story)
        return read_flag(flags.story, row.flag, Boundary::StoryFlag) != 0;
    if (row.source == FlagSource::Cutscene) {
        const auto byte = read_flag(flags.cutscene, row.flag, Boundary::CutsceneFlag);
        const auto signed_value = byte < 128 ? std::int32_t(byte) : std::int32_t(byte) - 256;
        return signed_value != 0;
    }
    return false;
}
}
std::int32_t value(std::int32_t character, const Catalog &catalog, Flags flags) {
    if (character < 0 || static_cast<std::size_t>(character) >= catalog.size())
        throw Error(Boundary::Character, character);
    std::int32_t result = 0;
    for (const auto &row : catalog[static_cast<std::size_t>(character)]) {
        // Original order wins: this is the last satisfied authored row, not a
        // popcount or a maximum over the numeric values.
        if (completed(row, flags))
            result = row.value;
    }
    return result;
}
} // namespace sadx::character_completion
