#include "player/player_plane.hpp"
#include <cstring>
extern "C" int sadx_native_player_plane(std::uint32_t mode,const std::uint32_t* words,
        std::uint32_t alias,std::uint16_t cw,float threshold,std::uint32_t* out){
    if(!words||!out||alias>3||mode>1)return 1;
    static_assert(sizeof(sadx::NinjaPlane)==24);
    sadx::NinjaPlane planes[3];std::memcpy(planes,words,72);
    std::memset(out,0,96);
    try{
        sadx::X87Context fp(cw);const sadx::PlayerPlaneConstants c{{0,1},threshold,1};
        if(mode==0)out[18]=sadx::source_planes_parallel(planes[0],planes[1],c,fp);
        else{
            auto* output=alias==3?nullptr:&planes[alias==0?2:alias-1];
            const auto value=sadx::intersect_source_planes(planes[0],planes[1],output,c,fp);
            out[19]=value.significand();out[20]=value.significand()>>32;out[21]=value.sign_exponent();
        }
        std::memcpy(out,planes,72);out[22]=fp.exception_flags();out[23]=fp.control_word();
        return 0;
    }catch(const sadx::X87Error&){return 2;}
}
