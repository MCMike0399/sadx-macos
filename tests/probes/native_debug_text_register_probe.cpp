#include "ninja/ninja_debug_text.hpp"
#include <algorithm>
// Native 793e70 with the layout of tools/verify_original_debug_text_register.py.
// in: [0] 3ce712c [1..32] 3d0fa40 slots   out: [0] completed [1] 3ce712c [2..33] slots [34] EAX
extern "C" int sadx_native_debug_text_register(const std::uint32_t* in,std::uint32_t* out){
    using namespace sadx;
    NinjaCallbackList list;list.count_at_3ce712c=in[0];list.slots_at_3d0fa40.assign(32,std::nullopt);for(int i=0;i<32;++i)list.slots_at_3d0fa40[i]=in[1+i];
    const auto r=run_debug_text_register_793e70(list);
    std::fill(out,out+40,0);out[0]=r?1U:0U;out[1]=list.count_at_3ce712c.value_or(0xdeadbeef);
    for(std::size_t i=0;i<32&&i<list.slots_at_3d0fa40.size();++i)out[2+i]=list.slots_at_3d0fa40[i].value_or(0xdeadbeef);
    out[34]=0;return 1;
}
