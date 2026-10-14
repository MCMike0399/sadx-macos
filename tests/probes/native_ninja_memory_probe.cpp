#include "engine/ninja_memory.hpp"
#include <algorithm>
extern "C" int sadx_native_ninja_memory(const std::uint32_t* c,std::uint32_t* out){
    using namespace sadx;std::fill(out,out+16,0U);NinjaMemoryState state{c[1]};
    const auto record=[&](std::uint32_t kind,std::uint32_t handle,std::uint32_t value,std::uint32_t receipt){const auto at=7+4*out[6]++;out[at]=kind;out[at+1]=handle;out[at+2]=value;out[at+3]=receipt;return std::optional<std::uint32_t>(receipt);};
    NinjaMemoryMethods methods{[&]{return record(1,0,0,c[3]);},[&](std::uint32_t h,std::uint32_t n){return record(2,h,n,c[4]);},[&](std::uint32_t h,std::uint32_t p){return record(3,h,p,c[5]);}};
    const auto r=c[0]==0?nj_malloc_780f60(state,methods,c[2]):nj_free_780fb0(state,methods,c[2]);
    out[0]=bool(r);out[1]=r.eax;out[2]=state.heap_at_389d628.value_or(0);out[3]=r.created;out[4]=static_cast<std::uint32_t>(r.failure);out[5]=r.source_boundary;return 1;
}
