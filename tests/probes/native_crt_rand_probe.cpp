#include "cpu/crt_rand.hpp"
// in: ops (0 = rand, else srand(value)), values; out: returns/seeds per op.
extern "C" int sadx_native_crt_rand(std::uint32_t initial,const std::uint32_t* ops,const std::uint32_t* values,std::size_t count,std::uint32_t* out){
    sadx::CrtRandState s;s.seed_at_2bcbdf8=initial;
    for(std::size_t i=0;i<count;++i){if(ops[i])sadx::crt_srand_6443b5(s,values[i]);else out[2*i]=*sadx::crt_rand_6443bf(s);out[2*i+1]=*s.seed_at_2bcbdf8;}
    return 1;
}
