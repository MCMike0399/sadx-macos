#include "engine/ninja_view_native.hpp"
#include <cstring>
extern "C" int sadx_native_projection_range(const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    if(!in||count!=23||!out||capacity<40)return 0;
    const auto number=[](std::uint32_t b){float v;std::memcpy(&v,&b,4);return v;};
    const auto bits=[](float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;};
    sadx::view::State state;state.camera.near_distance=number(in[2]);state.camera.far_distance=number(in[3]);
    sadx::NinjaMatrix matrix;for(unsigned i=0;i<16;++i)matrix[i]=number(in[7+i]);
    for(std::size_t i=0;i<capacity;++i)out[i]=0;
    sadx::view::ClipServices services{[&]{++out[7];return matrix;},[&](const sadx::NinjaMatrix& m){out[8]=1;for(unsigned i=0;i<16;++i)out[9+i]=bits(m[i]);}};
    sadx::X87Context fp(static_cast<std::uint16_t>(in[4]));
    sadx::view::set_clip_range(number(in[0]),number(in[1]),state,services,fp);
    out[0]=1;out[1]=bits(state.near_distance);out[2]=bits(state.far_distance);
    out[3]=bits(state.negative_near_distance);out[4]=bits(state.negative_far_distance);
    out[5]=bits(state.camera.near_distance);out[6]=bits(state.camera.far_distance);out[25]=fp.exception_flags()&63;return 1;
}
