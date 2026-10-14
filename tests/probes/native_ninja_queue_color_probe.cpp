#include "engine/ninja_queue_consumer.hpp"
#include <algorithm>
#include <cstring>
// Native 402f40 (+ its 77ff90 tail) with the layout of tools/oracles/verify_original_ninja_queue_color.py.
// in:  [0] CW [1] 7dcb10 bits [2] 7dcb0c bits [3..6] the input colour [7..10] the prior 3ab9864 words
// out: [0] completed [1] source boundary [2..5] 3ab9864 [6..9] 3d0f7f0 [10] x87 exception flags
extern "C" int sadx_native_ninja_queue_color(const std::uint32_t* in,std::uint32_t* out){
    using namespace sadx;
    const auto f=[](std::uint32_t b){float v;std::memcpy(&v,&b,4);return v;};
    const auto u=[](float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;};
    std::optional<std::array<float,4>> color=std::array<float,4>{f(in[7]),f(in[8]),f(in[9]),f(in[10])},projection;
    NinjaQueueColorOwners owners{color,projection,f(in[1]),f(in[2])};
    NinjaQueueColorResult partial;X87Context fp(static_cast<std::uint16_t>(in[0]));std::uint32_t site=0;
    std::fill(out,out+11,0);
    try{
        if(!ninja_queue_color_402f40(std::array<float,4>{f(in[3]),f(in[4]),f(in[5]),f(in[6])},owners,partial,fp,site))return 1;
        out[0]=1;
    }catch(const X87Error&){out[1]=site;}
    for(unsigned i=0;i<4;++i){out[2+i]=color?u((*color)[i]):0;out[6+i]=projection?u((*projection)[i]):0;}
    out[10]=fp.exception_flags();
    return 1;
}
