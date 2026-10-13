#include "engine/ninja_fog.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
// Semantics the original-instruction oracle does not compare: explicit native
// boundaries, missing owners, unknown facts and partial state on render failure.
namespace {
using namespace sadx;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
float bits(std::uint32_t b){float v;std::memcpy(&v,&b,4);return v;}
struct Fixture {
    NinjaQueueConsumerState state;NinjaFogDeviceState device;NinjaRenderState render;std::vector<NinjaRenderStateCommand> calls;std::size_t fail_at=0;X87Context fp{0x7f};
    NinjaFogTable table{};std::optional<std::int32_t> display=0;std::optional<std::uint8_t> flag=0;
    std::optional<float> level_start=300.0F,level_end=-3000.0F;std::optional<std::uint32_t> level_color=0x123456;
    Fixture(){
        render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});
        render.set_render_state=[this](const NinjaRenderStateCommand& c){calls.push_back(c);return NinjaRenderCallResult{calls.size()!=fail_at,{}};};
        state.fog_enabled_at_3d12798=1;state.fog_mode_at_3d0849c=1;state.fog_profile_at_3abdc6c=0;state.special_flags_at_909eb4=0;
        state.cached_fog_start_at_90a098=65535.0F;state.cached_fog_end_at_90a09c=65535.0F;
        device.start_value_at_3d0fac0=0.0F;device.end_value_at_3d12894=0.0F; // zero-filled .data in the load image
    }
    NinjaFogDeviceOwners owners(){return {state,device,&render};}
    NinjaFogResult init(){auto o=owners();return ninja_fog_initialize_794ec0(o,{0x00100000U,0U},fp);}
    NinjaFogOwners fog(bool with_device){return {state,flag,&display,&render,bits(0xc77fff00),with_device?&device:nullptr,with_device?&table:nullptr,&level_start,&level_end,&level_color};}
};
}
int main(){try{
    {Fixture f;const auto r=f.init();
        require(r&&f.state.fog_enabled_at_3d12798==0u&&f.state.fog_supported_at_3d128a4==1u&&f.device.end_index_at_3d0fce0==0x7eu&&f.device.start_index_at_3d12950==0x7fu,"794ec0 stores");
        require(f.calls.size()==8&&f.calls[0].state==28&&f.calls[0].source_call==0x794ed5&&f.calls[4].source_call==0x794fed&&f.calls[5].state==35&&f.calls[5].source_call==0x794ba5&&f.calls[6].value==0x477ffe00,"794ec0 then its 794db0 -> 7949f0 tail");
        require(f.device.w_at_3d12a00[0]==1.0F&&f.device.w_at_3d12a00[127]==248.0F&&f.device.reciprocal_at_3d12c00[0]==32768.0F,"exact w table and reciprocals");}
    {Fixture f;f.state.fog_supported_at_3d128a4=7;auto o=f.owners();const auto r=ninja_fog_initialize_794ec0(o,{0x00100000U,0x20U},f.fp);
        require(r&&f.state.fog_supported_at_3d128a4==0u&&f.device.software_density_at_3d128a0==0x3f800000u&&f.calls.size()==1,"CTRLFLAG0 bit 0x20 disables table fog");}
    {Fixture f;auto o=f.owners();const auto r=ninja_fog_initialize_794ec0(o,{std::nullopt,0U},f.fp);
        require(r.failure==NinjaFogFailure::unknown_fact&&r.required_fact==0x3d127e4u&&f.state.fog_enabled_at_3d12798==0u,"unknown RasterCaps stops after the fog-off call");}
    {Fixture f;f.render.device_at_3d128b0.reset();auto o=f.owners();const auto r=ninja_fog_initialize_794ec0(o,{0x00100000U,0U},f.fp);
        require(r.failure==NinjaFogFailure::render_failed&&r.source_boundary==0x794ec1&&f.state.fog_enabled_at_3d12798==1u,"unknown device stops before the 794ecc store");}
    {Fixture f;f.fail_at=2;const auto r=f.init();
        require(r.failure==NinjaFogFailure::render_failed&&r.source_boundary==0x794fa1&&!f.device.end_index_at_3d0fce0&&r.render.pending_command,"renderer failure keeps earlier stores only");}
    {Fixture f;require(bool(f.init()),"init");f.calls.clear();auto o=f.fog(false);const auto r=ninja_fog_411af0_411b40(true,o,f.fp);
        require(r.failure==NinjaFogFailure::unlifted_callee&&r.source_boundary==0x4119d8&&r.callee_boundary==0x787240&&f.calls.empty(),"411af0 without a fog device stops at 787240");}
    {Fixture f;require(bool(f.init()),"init");auto o=f.fog(true);const auto r=ninja_fog_411af0_411b40(true,o,f.fp);
        require(r&&f.state.cached_fog_start_at_90a098==bits(0xc77fff00)&&f.table[0]==0.0F&&f.table[127]==0.0F&&f.state.fog_mode_at_3d0849c==0u,"4119b0 update: zeroed table, cache, fog off");}
    {Fixture f;require(bool(f.init()),"init");f.state.fog_profile_at_3abdc6c=1;auto o=f.fog(true);const auto r=ninja_fog_411af0_411b40(true,o,f.fp);
        require(r&&f.state.cached_fog_start_at_90a098==-300.0F&&f.state.cached_fog_end_at_90a09c==-3000.0F&&f.device.color_at_3d12898==0x123456u&&f.state.fog_enabled_at_3d12798==1u&&f.flag==1,"411a20 level fog then fog on");}
    {Fixture f;require(bool(f.init()),"init");NinjaFogTable t{};for(auto& v:t)v=0.0F;auto o=f.owners();
        const auto r=ninja_fog_generate_table_787260(t,0.0F,bits(0x7f800000),o,f.fp);
        require(r.failure==NinjaFogFailure::unmodeled_operation&&r.source_boundary==0x78728e&&r.callee_boundary==0x7b5eef,"frexp of Inf is an explicit boundary");}
    {Fixture f;require(bool(f.init()),"init");NinjaFogTable t{};auto o=f.owners();const auto r=ninja_fog_generate_table_787260(t,-1.0F,-250.0F,o,f.fp);
        require(r.failure==NinjaFogFailure::index_out_of_range&&r.source_boundary==0x78753a&&t[127],"reference past the table is explicit; generated entries remain");}
    {Fixture f;require(bool(f.init()),"init");NinjaFogTable t{};for(auto& v:t)v=0.0F;t[0]=0.2F;t[1]=0.1F;f.device.reciprocal_at_3d12c00[0]=1.0F;f.device.reciprocal_at_3d12c00[1]=3.0F;
        auto o=f.owners();f.calls.clear();const auto r=ninja_fog_set_table_794e40(t,o,f.fp);
        require(r&&f.device.start_index_at_3d12950==0u&&f.device.end_index_at_3d0fce0==2u&&f.device.start_value_at_3d0fac0==0.2F&&f.device.end_value_at_3d12894==0.0F&&f.calls.size()==3,"table edges");
        NinjaFogTable exp2{};for(auto& v:exp2)v=0.0F;exp2[0]=0.5F;exp2[0x7e]=0.1F;exp2[0x7f]=-0.75F;f.calls.clear();
        const auto e=ninja_fog_set_table_794e40(exp2,o,f.fp);
        require(e&&f.calls.size()==2&&f.calls[0].state==35&&f.calls[0].value==2&&f.calls[0].source_call==0x794b20&&f.calls[1].state==38&&f.calls[1].source_call==0x794b35,"EXP2 mode and density commands");
        require((f.calls[1].value&0x7fffffffU)>0x7f800000U,"EXP2 negative square-root density preserves NaN");}
    {Fixture f;f.display=1;auto o=f.fog(true);const auto r=ninja_fog_411af0_411b40(false,o,f.fp);require(r&&f.flag==0&&f.calls.empty()&&f.state.fog_enabled_at_3d12798==1u,"display switch returns at once");}
    std::cout<<"Ninja fog: device init stores/trace, CTRLFLAG0, unknown caps/device, partial render failure, missing fog-device owner, 4119b0 and 411a20 updates, EXP2 mode/density and explicit frexp/past-table boundaries passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
