#pragma once

#include "engine/ninja_matrix.hpp"
#include "cpu/fidelity_float.hpp"

namespace sadx {
// FUNCTION: SONIC 0x784be0 njTranslate
void translate_ninja_matrix(NinjaMatrix &, float x, float y, float z, fidelity::Context &);

// FUNCTION: SONIC 0x788ad0 njMultiMatrix
void multiply_ninja_matrix(NinjaMatrix &destination, const NinjaMatrix &source, fidelity::Context &);

// FUNCTION: SONIC 0x781770 njRotateXYZ
void rotate_ninja_xyz(NinjaMatrixPrimitives &, NinjaMatrix &, const std::array<std::int32_t, 3> &,
                      fidelity::Context &);
// FUNCTION: SONIC 0x7819c0 njRotateZYX
void rotate_ninja_zyx(NinjaMatrixPrimitives &, NinjaMatrix &, const std::array<std::int32_t, 3> &,
                      fidelity::Context &);
} // namespace sadx
