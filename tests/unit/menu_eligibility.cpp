#include "game/menu_eligibility.hpp"
#include <cassert>
int main() {
  sadx::save::Data save{};
  std::array<std::uint8_t, 512> flags{};
  std::uint8_t override = 0, tutorial = 1;
  sadx::menu_eligibility::Progress progress{save, flags, override, tutorial};
  assert(sadx::menu_eligibility::character_enabled(6, progress) == 0);
  for (unsigned bit = 0; bit < 130; ++bit)
    save.emblems[bit / 8] |= 1U << (bit % 8);
  assert(sadx::menu_eligibility::emblem_count(save) == 130);
  assert(sadx::menu_eligibility::character_enabled(6, progress) == 1 &&
         flags[89] == 1);
  save.emblems.fill(0);
  assert(sadx::menu_eligibility::metal_sonic_enabled(progress) == 1);
  flags[65] = 255;
  assert(sadx::menu_eligibility::character_enabled(0, progress) == 1);
  bool unknown = false;
  try {
    sadx::menu_eligibility::character_enabled(99, progress);
  } catch (const sadx::menu_eligibility::UnknownCharacter &e) {
    unknown = e.character() == 99;
  }
  assert(unknown);
  override = 255;
  assert(sadx::menu_eligibility::character_enabled(99, progress) == 1);
}
