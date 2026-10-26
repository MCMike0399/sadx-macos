#include "math/ninja_length.hpp"

namespace sadx {
fidelity::ExactFloat vector_length(const NinjaVector &vector, fidelity::Context &context) {
    const fidelity::ExactFloat x(context, vector.x);
    const auto xx = x * x;
    const fidelity::ExactFloat y(context, vector.y);
    const auto yy = y * y;
    const fidelity::ExactFloat z(context, vector.z);
    const auto zz = z * z;
    const float squared = ((zz + yy) + xx).rounded();
    return fidelity::ExactFloat::signed_square_root(context, squared);
}
} // namespace sadx
