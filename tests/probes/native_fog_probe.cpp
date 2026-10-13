#include "engine/ninja_fog.hpp"
#include <algorithm>
#include <cstring>
// Native Ninja fog owners with the layout of tools/verify_original_fog.py.
// in[0] kind (0 794ec0, 1 7949b0, 2 787260, 3 794e40, 4 411af0/411b40),
// in[1] CW, in[2] fail-at render call (0 none), in[4..26] globals/arguments,
// in[32..159] 3d12a00, in[160..287] 3d12c00, in[288..415] 3b10f78.
namespace {
float as_float(std::uint32_t b){float v;std::memcpy(&v,&b,4);return v;}
std::uint32_t as_bits(const std::optional<float>& v){if(!v)return 0xdeadbeef;std::uint32_t b;std::memcpy(&b,&*v,4);return b;}
}
extern "C" int sadx_native_fog(const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    if(!in||count!=416||!out||capacity<480)return 0;
    sadx::NinjaQueueConsumerState s;s.fog_enabled_at_3d12798=in[4];s.fog_supported_at_3d128a4=in[5];s.fog_mode_at_3d0849c=in[12];
    s.fog_profile_at_3abdc6c=static_cast<std::uint8_t>(in[18]);s.special_flags_at_909eb4=static_cast<std::uint8_t>(in[19]);
    s.cached_fog_start_at_90a098=as_float(in[20]);s.cached_fog_end_at_90a09c=as_float(in[21]);
    sadx::NinjaFogDeviceState d;d.software_density_at_3d128a0=in[6];d.color_at_3d12898=in[7];d.end_index_at_3d0fce0=in[8];d.start_index_at_3d12950=in[9];
    d.start_value_at_3d0fac0=as_float(in[10]);d.end_value_at_3d12894=as_float(in[11]);
    sadx::NinjaFogTable table;
    for(unsigned i=0;i<128;++i){d.w_at_3d12a00[i]=as_float(in[32+i]);d.reciprocal_at_3d12c00[i]=as_float(in[160+i]);table[i]=as_float(in[288+i]);}
    std::uint32_t calls=0;
    sadx::NinjaRenderState render;render.device_at_3d128b0=std::make_shared<sadx::NinjaRenderDevice>(sadx::NinjaRenderDevice{1});
    render.set_render_state=[&](const sadx::NinjaRenderStateCommand&){++calls;return sadx::NinjaRenderCallResult{!(in[2]&&calls==in[2]),{}};};
    sadx::X87Context fp(static_cast<std::uint16_t>(in[1]));
    sadx::NinjaFogDeviceOwners device{s,d,&render};
    std::optional<std::int32_t> display=static_cast<std::int32_t>(in[17]);std::optional<std::uint8_t> flag=static_cast<std::uint8_t>(in[25]);
    std::optional<float> level_start=as_float(in[22]),level_end=as_float(in[23]);std::optional<std::uint32_t> level_color=in[24];
    sadx::NinjaFogResult r;
    switch(in[0]){
    case 0:r=sadx::ninja_fog_initialize_794ec0(device,{in[13],in[14]},fp);break;
    case 1:r=sadx::ninja_fog_color_7949b0(in[15],device);break;
    case 2:r=sadx::ninja_fog_generate_table_787260(table,as_float(in[15]),as_float(in[16]),device,fp);break;
    case 3:r=sadx::ninja_fog_set_table_794e40(table,device,fp);break;
    case 4:{sadx::NinjaFogOwners o{s,flag,&display,&render,as_float(0xc77fff00),&d,&table,&level_start,&level_end,&level_color};r=sadx::ninja_fog_411af0_411b40(in[26]!=0,o,fp);break;}
    default:return 0;
    }
    std::fill(out,out+capacity,0);
    out[0]=r?1U:0U;out[1]=static_cast<std::uint32_t>(r.render.commands.size());
    const std::uint32_t globals[]={s.fog_enabled_at_3d12798.value_or(0xdeadbeef),s.fog_supported_at_3d128a4.value_or(0xdeadbeef),d.software_density_at_3d128a0.value_or(0xdeadbeef),
        d.color_at_3d12898.value_or(0xdeadbeef),d.end_index_at_3d0fce0.value_or(0xdeadbeef),d.start_index_at_3d12950.value_or(0xdeadbeef),as_bits(d.start_value_at_3d0fac0),as_bits(d.end_value_at_3d12894),
        s.fog_mode_at_3d0849c.value_or(0xdeadbeef),as_bits(s.cached_fog_start_at_90a098),as_bits(s.cached_fog_end_at_90a09c),flag.value_or(0xef)};
    std::copy(std::begin(globals),std::end(globals),out+2);
    out[14]=fp.exception_flags()&0x3f;out[15]=r.source_boundary;
    for(std::size_t i=0;i<r.render.commands.size()&&i<24;++i){const auto& c=r.render.commands[i];out[16+3*i]=c.state;out[17+3*i]=c.value;out[18+3*i]=c.source_call;}
    for(unsigned i=0;i<128;++i){out[96+i]=as_bits(d.reciprocal_at_3d12c00[i]);out[224+i]=as_bits(d.w_at_3d12a00[i]);out[352+i]=as_bits(table[i]);}
    return 1;
}
