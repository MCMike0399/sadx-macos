#include "player/player_projection.hpp"
#include <cstddef>
#include <cstring>
namespace {
float number(std::uint32_t w){float v;std::memcpy(&v,&w,4);return v;}
std::uint32_t bits(float v){std::uint32_t w;std::memcpy(&w,&v,4);return w;}
sadx::NinjaVector vector(const std::uint32_t* w){return {number(w[0]),number(w[1]),number(w[2])};}
}
extern "C" int sadx_native_projection_cross(const std::uint32_t* w,std::uint16_t cw,std::uint32_t* out){
    if(!w||!out)return 1;
    try{
        const auto a=vector(w),b=vector(w+3);sadx::NinjaVector cross;
        sadx::X87Context fp(cw);const auto length=sadx::cross_product_length_extended(a,b,cross,fp);
        out[0]=bits(cross.x);out[1]=bits(cross.y);out[2]=bits(cross.z);
        out[3]=length.sign_exponent();out[4]=static_cast<std::uint32_t>(length.significand());
        out[5]=static_cast<std::uint32_t>(length.significand()>>32U);out[6]=fp.exception_flags();return 0;
    }catch(const sadx::X87Error&){return 2;}
}
extern "C" int sadx_native_projection_angle(const std::uint8_t* constants,std::size_t constants_size,
    const std::uint32_t* w,std::size_t count,std::uint16_t cw,std::uint32_t* out,std::size_t out_count){
    if(!constants||constants_size!=28||!w||count!=12||!out||out_count!=15)return 1;
    try{
        sadx::CoordinateAxisConstants axis;std::memcpy(&axis.zero,constants,4);std::memcpy(&axis.one,constants+4,4);
        sadx::ProjectionAngleConstants c;std::memcpy(&c.negative_one,constants+8,4);
        std::memcpy(&c.first_factor,constants+12,8);std::memcpy(&c.second_factor,constants+20,8);
        sadx::CrtAsinRuntime runtime;if(w[10])runtime.matherr_control=w[11];
        const auto a=vector(w),b=vector(w+3),side=vector(w+6);sadx::X87Context fp(cw);
        const auto result=sadx::projection_vector_angle(a,b,w[9]?&side:nullptr,axis,c,runtime,fp);
        std::memset(out,0,15*4);if(result.angle)out[0]=static_cast<std::uint32_t>(*result.angle);
        out[1]=static_cast<std::uint32_t>(result.failure);out[2]=result.source_boundary;
        out[3]=static_cast<std::uint32_t>(result.crt_failure);out[4]=result.nested_source_boundary;
        out[5]=bits(result.stored_dot);out[6]=result.cross.has_value();
        if(result.cross){out[7]=bits(result.cross->x);out[8]=bits(result.cross->y);out[9]=bits(result.cross->z);}
        out[10]=fp.exception_flags();out[11]=fp.control_word();out[12]=result.pending_stored_result.has_value();
        if(result.pending_stored_result){std::uint64_t raw;std::memcpy(&raw,&*result.pending_stored_result,8);out[13]=static_cast<std::uint32_t>(raw);out[14]=static_cast<std::uint32_t>(raw>>32U);}return 0;
    }catch(const sadx::X87Error&){return 2;}
}
