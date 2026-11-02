#include "engine/ninja_matrix_composition.hpp"
#include "cpu/fidelity_float.hpp"

namespace sadx {
using fidelity::ExactFloat;

void translate_ninja_matrix(NinjaMatrix &matrix, float x, float y, float z, fidelity::Context &fp) {
    for (unsigned column = 0; column < 4; ++column) {
        // Unlike njTranslateV, the numeric entry accumulates X/Y before Z in
        // columns one through three. Column zero accumulates Y/Z before X.
        auto first = ExactFloat(fp, column == 0 ? y : x) * matrix[column == 0 ? 4 : column];
        auto second = ExactFloat(fp, column == 0 ? z : y) * matrix[column == 0 ? 8 : 4 + column];
        auto sum = first + second;
        auto third = ExactFloat(fp, column == 0 ? x : z) * matrix[column == 0 ? 0 : 8 + column];
        matrix[12 + column] = (sum + third + matrix[12 + column]).rounded();
    }
}

void multiply_ninja_matrix(NinjaMatrix &destination, const NinjaMatrix &source, fidelity::Context &fp) {
    const NinjaMatrix original = destination;
    // The source snapshots only the destination. If the operands alias, later
    // reads of source see the components already written by this operation.
    for (unsigned row = 0; row < 4; ++row) {
        for (unsigned column = 0; column < 4; ++column) {
            const auto term = [&](unsigned inner) {
                return ExactFloat(fp, original[inner * 4 + column]) * source[row * 4 + inner];
            };
            // Addition order varies across the unrolled original. Keep each
            // row's accumulation order, including the fourth-column exception.
            auto sum = column == 3 ? term(3) : term(1);
            sum = sum + (column == 3 ? term(row == 0 ? 0 : 1) : term(3));
            sum = sum + (column == 3 ? term(row == 0 ? 1 : 0) : term(row == 0 ? 0 : 2));
            sum = sum + (column == 3 ? term(2) : term(row == 0 ? 2 : 0));
            destination[row * 4 + column] = sum.rounded();
        }
    }
}

void rotate_ninja_xyz(NinjaMatrixPrimitives &coordinates, NinjaMatrix &matrix,
                      const std::array<std::int32_t, 3> &angles, fidelity::Context &fp) {
    // The numeric wrapper applies Z, then Y, then X, and skips an axis only
    // when its full signed angle is zero (not when its low word is zero).
    if (angles[2] != 0)
        coordinates.rotate_z(matrix, angles[2], fp);
    if (angles[1] != 0)
        coordinates.rotate_y(matrix, angles[1], fp);
    if (angles[0] != 0)
        coordinates.rotate_x(matrix, angles[0], fp);
}
} // namespace sadx
