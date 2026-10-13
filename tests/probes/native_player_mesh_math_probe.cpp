#include "player/player_mesh_math.hpp"
#include "player/player_mesh_matrix.hpp"
#include <cstring>

namespace {
float number(std::uint32_t w){float f;std::memcpy(&f,&w,4);return f;}
sadx::NinjaVector vector(const std::uint32_t* w){return {number(w[0]),number(w[1]),number(w[2])};}
}
// mode0 triangle normal,1 normalize-zero,2 normalize-up,3 inverse,4 determinant,
// 5 scale-vector. Input16words is shared fixed storage; onlysourceusedslotsread.
extern "C" int sadx_native_player_mesh_math(std::uint32_t mode,const std::uint32_t* words,
    const std::uint32_t* axis,std::uint16_t cw,std::uint32_t* output){
    if(!words || !axis || !output)return -1;
    try {
        std::memset(output,0,20*4);sadx::X87Context fp(cw);const sadx::CoordinateAxisConstants constants{number(axis[0]),number(axis[1])};
        if(mode==0){auto a=vector(words),b=vector(words+3),c=vector(words+6);sadx::NinjaVector result;
            sadx::mesh_triangle_normal(a,b,c,result,fp);std::memcpy(output,&result,12);}
        else if(mode==1 || mode==2){auto result=vector(words);const sadx::CrtAsinRuntime runtime{1,{}};
            const auto normalized=mode==1?sadx::normalize_mesh_vector_zero(result,constants,runtime,fp):sadx::normalize_mesh_vector_up(result,constants,runtime,fp);
            if(!normalized){output[19]=static_cast<std::uint32_t>(normalized.sqrt->failure);return 1;}
            std::memcpy(output,&result,12);}
        else if(mode>=3 && mode<=5){sadx::NinjaMatrix matrix;std::memcpy(matrix.data(),words,64);
            if(mode==3){output[16]=sadx::invert_mesh_matrix(matrix,constants,fp)?1:0;std::memcpy(output,matrix.data(),64);}
            else if(mode==4){const auto value=sadx::mesh_matrix_determinant(matrix,fp);output[0]=value.sign_exponent();output[1]=static_cast<std::uint32_t>(value.significand());output[2]=static_cast<std::uint32_t>(value.significand()>>32);}
            else{sadx::scale_mesh_matrix_vector(matrix,vector(words+16),fp);std::memcpy(output,matrix.data(),64);}
        }else return -2;
        output[17]=fp.exception_flags();output[18]=fp.control_word();return 0;
    }catch(const sadx::X87Error&){return -3;}
}
extern "C" const char* sadx_native_player_mesh_math_compiler(){return "Clang " __clang_version__ "; ARM64 C++17 O2 software x87; static ordered matrix expressions";}
