#include "game/menu_eligibility.hpp"
#include <algorithm>
extern "C" std::int32_t sadx_native_menu_eligibility(const std::uint8_t *input,
                                                     std::uint32_t kind,
                                                     std::uint32_t character,
                                                     std::uint8_t *output) {
  sadx::save::Data save{};
  std::copy_n(input, 17, save.emblems.begin());
  std::array<std::uint8_t, 512> flags{};
  std::copy_n(input + 17, 512, flags.begin());
  const auto override = input[529], tutorial = input[530];
  sadx::menu_eligibility::Progress progress{save, flags, override, tutorial};
  std::int32_t value;
  try {
    value = kind == 0   ? sadx::menu_eligibility::emblem_count(save)
            : kind == 1 ? sadx::menu_eligibility::metal_sonic_enabled(progress)
                        : sadx::menu_eligibility::character_enabled(character,
                                                                    progress);
  } catch (const sadx::menu_eligibility::UnknownCharacter &unknown) {
    if (unknown.character() != character)
      return -998;
    value = -999;
  }
  std::copy(flags.begin(), flags.end(), output);
  return value;
}
