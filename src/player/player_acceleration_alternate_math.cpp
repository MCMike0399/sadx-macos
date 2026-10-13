#include "player/player_acceleration_alternate_math.hpp"
#include "cpu/crt_integer.hpp"
#include "cpu/x87_transcendental.hpp"

namespace sadx {

void event_vector_subtract(NinjaVector& dst,const NinjaVector& other,X87Context& fp) {
    dst.x=fp.to_float(fp.subtract_float_memory(fp.from_float(dst.x),other.x));
    dst.y=fp.to_float(fp.subtract_float_memory(fp.from_float(dst.y),other.y));
    dst.z=fp.to_float(fp.subtract_float_memory(fp.from_float(dst.z),other.z));
}

void event_vector_add(NinjaVector& dst,const NinjaVector& other,X87Context& fp) {
    // 0x785170 changes operand order after X; a generic vector '+' would lose
    // the source FLD-versus-memory signaling-NaN selection for Y and Z.
    dst.x=fp.to_float(fp.add_float_memory(fp.from_float(dst.x),other.x));
    dst.y=fp.to_float(fp.add_float_memory(fp.from_float(other.y),dst.y));
    dst.z=fp.to_float(fp.add_float_memory(fp.from_float(other.z),dst.z));
}

std::uint32_t event_horizontal_heading(const NinjaVector& vector,double units,
                                       double factor,X87Context& fp) {
    const auto y=fp.from_float(vector.z);
    const auto x=fp.from_float(vector.x);
    const auto atan=X87Transcendental::fpatan_bochs_p5_p6(fp,y,x);
    const auto scaled=fp.multiply_double_memory(atan,units);
    const auto angle=fp.multiply_double_memory(scaled,factor);
    return crt_ftol_low32(angle,fp)&0xffffU;
}

} // namespace sadx
