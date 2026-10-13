#include "ninja/ninja_projection_range.hpp"
#include <cstring>
// in: 0 near,1 far,2 cache x,3 cache y,4 cw,5 -1.0f,6 limit,7..22 matrix
extern "C" int sadx_native_projection_range(const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    if(!in||count!=23||!out||capacity<40)return 0;
    const auto f=[](std::uint32_t b){float v;std::memcpy(&v,&b,4);return v;};const auto bits=[](float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;};
    sadx::NinjaProjectionRangeState s;s.cache_x_at_3aad0c0=f(in[2]);s.cache_y_at_3aad0c4=f(in[3]);s.negative_one_at_7dcb0c=f(in[5]);s.limit_at_7dce50=f(in[6]);
    sadx::NinjaMatrix m;for(unsigned i=0;i<16;++i)m[i]=f(in[7+i]);std::optional<sadx::NinjaMatrix> written;std::uint32_t gets=0;
    sadx::NinjaTransformOwner t{[&](std::uint32_t st)->std::optional<sadx::NinjaMatrix>{++gets;if(st!=3)return std::nullopt;return m;},[&](std::uint32_t st,const sadx::NinjaMatrix& x){if(st!=3)return false;written=x;return true;}};
    sadx::X87Context fp(static_cast<std::uint16_t>(in[4]));const auto r=sadx::ninja_set_projection_range_403180(f(in[0]),f(in[1]),s,t,fp);
    for(std::size_t i=0;i<capacity;++i)out[i]=0;
    out[0]=r.completed;out[1]=bits(*s.near_at_3d0f808);out[2]=bits(*s.far_at_3d0f968);out[3]=bits(*s.x_at_90082c);out[4]=*s.y_bits_at_900830;out[5]=bits(*s.cache_x_at_3aad0c0);out[6]=bits(*s.cache_y_at_3aad0c4);
    out[7]=gets;out[8]=written.has_value();if(written)for(unsigned i=0;i<16;++i)out[9+i]=bits((*written)[i]);out[25]=fp.exception_flags()&0x3f;return 1;
}
