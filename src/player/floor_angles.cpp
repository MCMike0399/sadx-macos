#include "player/floor_angles.hpp"
#include "cpu/crt_integer.hpp"
#include <cstring>

namespace sadx {
namespace {
X87Value absolute(X87Value value) { return value.negative() ? value.negated() : value; }
std::int32_t signed_word(std::uint32_t word) {
    std::int32_t result;
    std::memcpy(&result,&word,4);
    return result;
}
}

FloorAngleArithmeticError::FloorAngleArithmeticError(const X87Error& error,std::int32_t inclination,
                                                     std::uint32_t boundary)
    : X87Error(error),inclination_(inclination),boundary_(boundary) {}

FloorAnglesResult floor_normal_angles(const NinjaVector& normal,const FloorAngleConstants& constants,
                                      const CrtAsinRuntime& runtime,X87Context& fp) {
    const auto compare=[&](X87Value value,float bound) { return fp.compare_float_memory(value,bound); };
    if(compare(fp.from_float(normal.z),constants.upper_cut)==X87Comparison::greater)
        return {CrtAsinFailure::none,NormalAngles{0x4000,0},0};
    const auto abs_y=absolute(fp.from_float(normal.y));
    if(compare(abs_y,constants.axis_epsilon)==X87Comparison::less &&
       compare(absolute(fp.from_float(normal.x)),constants.axis_epsilon)==X87Comparison::less &&
       compare(fp.from_float(normal.z),0)==X87Comparison::greater)
        return {CrtAsinFailure::none,NormalAngles{0x4000,0},0};
    if(compare(fp.from_float(normal.z),constants.lower_cut)==X87Comparison::less)
        return {CrtAsinFailure::none,NormalAngles{0xc000,0},0};
    // The source reuses the previously loaded absolute Y in this branch.
    if(compare(abs_y,constants.axis_epsilon)==X87Comparison::less &&
       compare(absolute(fp.from_float(normal.x)),constants.axis_epsilon)==X87Comparison::less &&
       compare(fp.from_float(normal.z),0)==X87Comparison::less)
        return {CrtAsinFailure::none,NormalAngles{0xc000,0},0};
    const auto inclination=crt_asin(fp.from_float(normal.z),runtime,fp);
    if(inclination.failure!=CrtAsinFailure::none || !inclination.value)
        return {inclination.failure,{},inclination.source_boundary};
    const auto scaled_inclination=fp.multiply_double_memory(
        fp.multiply_double_memory(*inclination.value,constants.angle_units),constants.positive_factor);
    const auto x=crt_ftol_low32(scaled_inclination,fp);
    const auto atan_x=fp.from_float(normal.x); // source FLD X before FLD Y
    const auto atan_y=fp.from_float(normal.y);
    const auto atan=X87Transcendental::fpatan_bochs_p5_p6(fp,atan_x,atan_y);
    const auto stored_inclination=signed_word(x); // source0x4b8f3a, before roll scaling
    std::uint32_t boundary=0x4b8f3c;
    try {
        const auto units=fp.multiply_double_memory(atan,constants.angle_units);
        boundary=0x4b8f42;
        const auto scaled_roll=fp.multiply_double_memory(units,constants.negative_factor);
        boundary=0x4b8f48;
        const auto z=crt_ftol_low32(scaled_roll,fp);
        return {CrtAsinFailure::none,NormalAngles{stored_inclination,signed_word(z)},0};
    } catch(const X87Error& error) {
        throw FloorAngleArithmeticError(error,stored_inclination,boundary);
    }
}
} // namespace sadx
