#pragma once
#include "input/input_mod_pad.hpp"
namespace sadx {
// NOTSA: maintained arithmetic coefficients for the pinned Input 2.6.1 profile.
// Settings and mutable controller data have separate owners; callers may supply variants.
InputModPadConstants input_mod_261_constants();
} // namespace sadx
