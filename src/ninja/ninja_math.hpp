#pragma once

#include <array>
#include <cstdint>
#include "cpu/x87_math.hpp"

namespace sadx {

// Shared source numeric constants used by axis rotation and vector-angle.
struct CoordinateAxisConstants {
    float zero; // source binary32 at0x7dcb2c
    float one;  // source binary32 at0x7dcb10
};

struct NinjaVector {
    float x;
    float y;
    float z;
};

// Reconstructed from the supplied sonic.exe, not an original-source recovery.
// The x86 routines return an x87 register. Extended methods retain that value;
// float methods perform an explicit binary32 store in the selected context.
// The caller owns the context and must supply its observed control word.
// Full-game control state and runtime equivalence remain unverified.
class NinjaMath {
public:
    using SineTable = std::array<float, 1025>;
    NinjaMath(SineTable quarter_sine_table, X87Context& context);

    // One revolution is 65536 angle units. All int32 inputs wrap to 16 bits.
    float sin(std::int32_t angle) const;
    float cos(std::int32_t angle) const;
    X87Value sin_extended(std::int32_t angle) const;
    X87Value cos_extended(std::int32_t angle) const;

private:
    SineTable quarter_sine_table_;
    X87Context& context_;
};

// Operation order follows the recovered instructions. Scalar_squared must
// not be replaced with dot(v,v), whose addition order is different.
X87Value inner_product_extended(const NinjaVector& first, const NinjaVector& second, X87Context& context);
X87Value scalar_squared_extended(const NinjaVector& vector, X87Context& context);
X87Value signed_sqrt_extended(float value, X87Context& context);
// Actual7889f0: first=ECX, second=EDX. Computes first×second using second
// FLD and first m32 operands, stores all components, then squares retained
// ext80 components for the binary32 squared-length spill/signed sqrt.
X87Value cross_product_length_extended(const NinjaVector& first,const NinjaVector& second,
                                      NinjaVector& output,X87Context&);
// Actual418ac0: cached load/square order, direct FSQRT, equality-only zero
// stores, and reciprocal/component m32 stores under the explicit source FP.
X87Value normalize_vector_extended(NinjaVector&,CoordinateAxisConstants,X87Context&);
float inner_product(const NinjaVector& first, const NinjaVector& second, X87Context& context);
float scalar_squared(const NinjaVector& vector, X87Context& context);
float signed_sqrt(float value, X87Context& context);

} // namespace sadx
