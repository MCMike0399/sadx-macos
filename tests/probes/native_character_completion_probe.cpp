#include "game/character_completion.hpp"
#include <algorithm>
namespace c = sadx::character_completion;
extern "C" void sadx_native_character_completion_probe(const std::uint32_t *input, std::uint32_t *out) {
    std::fill_n(out, 4, 0);
    c::Catalog catalog{};
    const auto character = static_cast<std::int32_t>(input[0]);
    const auto count = static_cast<std::int32_t>(input[1]);
    if (character >= 0 && character < 7)
        for (std::int32_t row = 0; row < count; ++row)
            catalog[static_cast<std::size_t>(character)].push_back({
                static_cast<std::int32_t>(input[8 + row * 3]),
                static_cast<c::FlagSource>(input[9 + row * 3]),
                static_cast<std::int32_t>(input[10 + row * 3])});
    std::array<std::uint8_t, 512> story{}, cutscene{};
    for (std::size_t index = 0; index < story.size(); ++index) {
        story[index] = static_cast<std::uint8_t>(input[200 + index]);
        cutscene[index] = static_cast<std::uint8_t>(input[712 + index]);
    }
    try { out[0] = static_cast<std::uint32_t>(c::value(character, catalog, {story, cutscene})); }
    catch (const c::Error &error) { out[1] = static_cast<std::uint32_t>(error.boundary()) + 1U; }
}
