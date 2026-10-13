#include "ninja/ninja_render.hpp"
#include <cstring>
extern "C" int sadx_native_ninja_render(const std::uint8_t* bytes,std::size_t size,const std::uint32_t* w,std::size_t words,std::uint32_t* out,std::size_t count){
    if(!bytes||(size!=56&&size!=88)||!w||words!=8||!out||count!=20)return 1;
    const auto table=sadx::decode_ninja_blend_tables(bytes,size);if(!table)return 2;
    sadx::NinjaRenderState state;state.blend_tables=&*table;
    if(w[3])state.packed_blend_at_3ab9890=static_cast<std::uint8_t>(w[2]);
    auto first=std::make_shared<sadx::NinjaRenderDevice>();first->identity=1;auto second=std::make_shared<sadx::NinjaRenderDevice>();second->identity=2;
    if(w[4])state.device_at_3d128b0=w[4]==1?first:sadx::NinjaRenderDeviceHandle{};
    unsigned called=0;
    state.set_render_state=[&](const auto&){const auto index=called++;std::int32_t receipt;const auto raw=w[5+index];std::memcpy(&receipt,&raw,4);if(index==0){if(w[7]==1)state.device_at_3d128b0=second;else if(w[7]==2)state.device_at_3d128b0.reset();}return sadx::NinjaRenderCallResult{true,receipt};};
    const auto result=sadx::ninja_set_blend(w[0],w[1],state);std::memset(out,0,count*4);out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;out[2]=state.packed_blend_at_3ab9890.has_value();if(state.packed_blend_at_3ab9890)out[3]=*state.packed_blend_at_3ab9890;out[4]=result.commands.size();out[5]=result.ignored_hresults.size();
    for(unsigned i=0;i<result.commands.size();++i){const auto& c=result.commands[i];out[6+i*4]=static_cast<std::uint32_t>(c.device->identity);out[7+i*4]=c.state;out[8+i*4]=c.value;out[9+i*4]=c.source_call;}
    for(unsigned i=0;i<result.ignored_hresults.size();++i)if(result.ignored_hresults[i]){out[14+i*2]=1;out[15+i*2]=static_cast<std::uint32_t>(*result.ignored_hresults[i]);}
    return 0;
}
