#include "ninja/ninja_queue_consumer.hpp"
#include <cstring>
extern "C" int sadx_native_fov(const float* table,const float* unit,std::int32_t angle,std::uint32_t width_bits,std::uint16_t cw,std::uint32_t* out){
    (void)unit;sadx::NinjaMath::SineTable t;std::memcpy(t.data(),table,4100);
    sadx::NinjaSpriteProjection projection{nullptr,{},{},{},{}};float width;std::memcpy(&width,&width_bits,4);sadx::X87Context fp(cw);
    const auto r=sadx::ninja_set_fov_7815c0(angle,t,projection,width,fp);
    out[0]=r.completed;out[1]=r.source_boundary;std::uint32_t b=0;if(projection.scale_at_3d0f9e0)std::memcpy(&b,&*projection.scale_at_3d0f9e0,4);out[2]=b;out[3]=fp.exception_flags()&0x3f;return 1;
}
