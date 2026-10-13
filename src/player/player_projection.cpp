#include "player/player_projection.hpp"
#include "cpu/crt_integer.hpp"
#include <cstring>
namespace sadx {
ProjectionAngleResult projection_vector_angle(const NinjaVector& a,const NinjaVector& b,
    const NinjaVector* sign,const CoordinateAxisConstants& axis,const ProjectionAngleConstants& c,
    const CrtAsinRuntime& runtime,X87Context& fp) {
    const auto dot=inner_product_extended(a,b,fp);
    ProjectionAngleResult result;result.stored_dot=fp.to_float(dot); // FST retains unrounded ST0
    const auto lower=fp.compare_float_memory(dot,c.negative_one);
    if(lower==X87Comparison::less||lower==X87Comparison::equal){result.angle=0x8000;return result;}
    const auto upper=fp.compare_float_memory(fp.from_float(result.stored_dot),axis.one);
    if(upper==X87Comparison::equal||upper==X87Comparison::greater){result.angle=0;return result;}
    const auto angle=crt_acos(fp.from_float(result.stored_dot),runtime,fp);
    if(angle.failure!=CrtAsinFailure::none||!angle.value){
        result.failure=ProjectionAngleFailure::acos_unavailable;result.source_boundary=0x438460;
        result.crt_failure=angle.failure;result.nested_source_boundary=angle.source_boundary;
        result.pending_stored_result=angle.pending_stored_result;result.pending_value=angle.value;return result;
    }
    const auto degrees=fp.multiply_double_memory(*angle.value,c.first_factor);
    std::uint32_t units=crt_ftol_low32(fp.multiply_double_memory(degrees,c.second_factor),fp);
    NinjaVector cross;(void)cross_product_length_extended(a,b,cross,fp);result.cross=cross;
    if(sign&&fp.compare_float_memory(inner_product_extended(*sign,cross,fp),axis.zero)==X87Comparison::less)
        units=0x10000U-units;
    std::int32_t signed_units;std::memcpy(&signed_units,&units,4);result.angle=signed_units;return result;
}
} // namespace sadx
