#include "ninja/ninja_render.hpp"
namespace sadx {
namespace {
std::uint32_t word(const std::uint8_t* p){return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
}
bool ninja_issue_render_state(NinjaRenderResult& result,NinjaRenderState& runtime,std::uint32_t state,std::uint32_t value,NinjaRenderSourceSites sites){
    auto reject=[&](NinjaRenderFailure f,std::uint32_t source){result.failure=f;result.source_boundary=source;return false;};
    if(!runtime.device_at_3d128b0)return reject(NinjaRenderFailure::unknown_device,sites.device_load);
    if(!*runtime.device_at_3d128b0)return reject(NinjaRenderFailure::null_device,sites.device_dereference);
    NinjaRenderStateCommand command{*runtime.device_at_3d128b0,state,value,sites.call};
    result.commands.push_back(command);
    if(!runtime.set_render_state){result.pending_command=command;return reject(NinjaRenderFailure::missing_renderer,sites.call);}
    const auto receipt=runtime.set_render_state(command);
    if(!receipt.completed){result.pending_command=command;return reject(NinjaRenderFailure::renderer_unavailable,sites.call);}
    result.ignored_hresults.push_back(receipt.original_hresult);return true;
}
bool ninja_issue_texture_stage_state(NinjaRenderStageResult& result,NinjaRenderState& runtime,std::uint32_t stage,std::uint32_t state,std::uint32_t value,NinjaRenderSourceSites sites){
    auto reject=[&](NinjaRenderFailure failure,std::uint32_t source){result.failure=failure;result.source_boundary=source;return false;};
    if(!runtime.device_at_3d128b0)return reject(NinjaRenderFailure::unknown_device,sites.device_load);
    if(!*runtime.device_at_3d128b0)return reject(NinjaRenderFailure::null_device,sites.device_dereference);
    NinjaRenderStageCommand command{*runtime.device_at_3d128b0,stage,state,value,sites.call};result.commands.push_back(command);
    if(!runtime.set_texture_stage_state){result.pending_command=command;return reject(NinjaRenderFailure::missing_renderer,sites.call);}
    const auto receipt=runtime.set_texture_stage_state(command);if(!receipt.completed){result.pending_command=command;return reject(NinjaRenderFailure::renderer_unavailable,sites.call);}
    result.ignored_hresults.push_back(receipt.original_hresult);return true;
}
std::optional<NinjaBlendTables> decode_ninja_blend_tables(const std::uint8_t* data,std::size_t size){
    if(!data||(size!=56&&size!=88))return {};
    NinjaBlendTables tables;for(unsigned i=0;i<2;++i)tables.states[i]=word(data+i*4);
    for(unsigned i=0;i<12;++i)tables.values[i]=word(data+8+i*4);
    if(size==88){std::array<std::uint32_t,4> following;for(unsigned i=0;i<4;++i)following[i]=word(data+56+i*4);tables.following_words_at_88ad9c=following;}
    return tables;
}
NinjaRenderResult ninja_set_blend(std::uint32_t selector,std::uint32_t value,NinjaRenderState& runtime){
    NinjaRenderResult result;
    if(selector<2){
        if(!runtime.packed_blend_at_3ab9890){result.failure=NinjaRenderFailure::unknown_packed_blend;result.source_boundary=selector==0?0x4030dd:0x403103;return result;}
        auto& packed=*runtime.packed_blend_at_3ab9890;
        if(selector==0)packed=static_cast<std::uint8_t>((packed&0xf0)|(value&15));
        else packed=static_cast<std::uint8_t>((packed&15)|((value&15)<<4));
    }
    return ninja_set_blend_77ec60(selector,value,runtime);
}
NinjaRenderResult ninja_set_blend_77ec60(std::uint32_t selector,std::uint32_t value,NinjaRenderState& runtime){
    NinjaRenderResult result;
    if(value<2){
        if(!ninja_issue_render_state(result,runtime,0x13,value==0?6:5,{value==0?0x77ec68U:0x77ec92U,value==0?0x77ec6dU:0x77ec97U,value==0?0x77ec74U:0x77ec9eU}))return result;
        ninja_issue_render_state(result,runtime,0x14,value==0?5:6,{value==0?0x77ec7aU:0x77eca4U,value==0?0x77ec7fU:0x77eca9U,value==0?0x77ec86U:0x77ecb0U});return result;
    }
    if(!runtime.blend_tables){result.failure=NinjaRenderFailure::missing_tables;result.source_boundary=0x77ecb7;return result;}
    const auto& tables=*runtime.blend_tables;const auto values=tables.values.size();
    if(value>=values+4||(value>=values&&!tables.following_words_at_88ad9c)){result.failure=NinjaRenderFailure::unsupported_value_index;result.source_boundary=0x77ecb7;return result;}
    // Source loads value first, then the device/vtable, then selector data.
    const auto mapped=value<values?tables.values[value]:(*tables.following_words_at_88ad9c)[value-values];
    if(!runtime.device_at_3d128b0){result.failure=NinjaRenderFailure::unknown_device;result.source_boundary=0x77ecbe;return result;}
    if(!*runtime.device_at_3d128b0){result.failure=NinjaRenderFailure::null_device;result.source_boundary=0x77ecc3;return result;}
    if(selector>=runtime.blend_tables->states.size()){result.failure=NinjaRenderFailure::unsupported_state_index;result.source_boundary=0x77ecca;return result;}
    ninja_issue_render_state(result,runtime,runtime.blend_tables->states[selector],mapped,{0x77ecbe,0x77ecc3,0x77ecd3});return result;
}
NinjaRenderResult ninja_set_depth_comparison(std::uint32_t value,const std::array<std::uint32_t,8>* table,NinjaRenderState& runtime){
    NinjaRenderResult result;
    if(!runtime.device_at_3d128b0){result.failure=NinjaRenderFailure::unknown_device;result.source_boundary=0x77ed04;return result;}
    if(!*runtime.device_at_3d128b0){result.failure=NinjaRenderFailure::null_device;result.source_boundary=0x77ed09;return result;}
    if(!table){result.failure=NinjaRenderFailure::missing_tables;result.source_boundary=0x77ed0e;return result;}
    ninja_issue_render_state(result,runtime,0x17,(*table)[value&7],{0x77ed04,0x77ed09,0x77ed19});return result;
}
NinjaRenderResult ninja_set_depth_write(std::uint32_t value,NinjaRenderState& runtime){
    NinjaRenderResult result;ninja_issue_render_state(result,runtime,0xe,value,{0x77ed24,0x77ed29,0x77ed2f});return result;
}
} // namespace sadx
