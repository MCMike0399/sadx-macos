#pragma once

#include "ninja/ninja_math.hpp"

namespace sadx {

// Source vector callees consumed by event acceleration. These are native
// source-ordered operations, including aliased output and NaN operand order.
void event_vector_subtract(NinjaVector& destination,const NinjaVector& other,X87Context&);
void event_vector_add(NinjaVector& destination,const NinjaVector& other,X87Context&);

// Event source FLD32 z, FLD32 x, FPATAN, FMUL64 twice and CRT _ftol, followed
// by the source low16 mask. Caller supplies the source binary64 scale/factor;
// the explicitly selected Bochs FPATAN model has unverified hardware fidelity.
std::uint32_t event_horizontal_heading(const NinjaVector&,double angle_units,
                                       double positive_factor,X87Context&);

} // namespace sadx
