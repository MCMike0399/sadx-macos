#pragma once

#include "cpu/fidelity_float.hpp"
#include "engine/graphics_frame.hpp"

namespace sadx::material_correction {

struct Context;

// [platform:d3d8] Device material operations; native and reference adapters share the algorithm.
graphics::Material get_material(Context &context);
void set_material(Context &context, const graphics::Material &material);

// NOTSA: borrow the live arithmetic context and source specular-power owner.
fidelity::ExactFloat exact(Context &context, float value);
float specular_power(Context &context);

// FUNCTION: SONIC 0x40a340 CorrectMaterial
void correct_material(Context &context);

} // namespace sadx::material_correction
