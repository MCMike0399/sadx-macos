#include "math/ninja_length.hpp"

namespace sadx {
fidelity::ExactFloat vector_length(const NinjaVector &vector, fidelity::Context &context) {
    return fidelity::ExactFloat::retained(
        context, fidelity::ExactFloat::vector_length_return(context, vector.x, vector.y, vector.z));
}
} // namespace sadx
