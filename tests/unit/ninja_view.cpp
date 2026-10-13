// engine/ninja_view: 781570 / 781250 / 791170. Exactness against the original bytes is the ninja-view oracle;
// this checks the documented shape: 781570's block, the default camera's view, the matrix-stack reset, the
// three device calls in order, the 389d398 copy / 389d358 inverse, and explicit failures for missing facts.
#include "engine/ninja_view.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

using namespace sadx;
namespace {
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
float fl(std::uint32_t b){float f;std::memcpy(&f,&b,4);return f;}
std::uint32_t bits(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
}
int main(int argc,char** argv){
    try{
        require(argc==2,"usage: ninja-view-test ninja-trig.f32");
        NinjaMath::SineTable table{};{std::ifstream in(argv[1],std::ios::binary);in.read(reinterpret_cast<char*>(table.data()),sizeof table);require(in.gcount()==sizeof table,"table");}
        NinjaViewState view;NinjaProjectionRangeState range{};NinjaSpriteProjection projection;NinjaScreenState screen;NinjaDeviceState device_state;
        ninja_view_init_781570(view,range);
        require(*view.camera_at_3aad0a0[5]==0xbf800000U && *view.camera_at_3aad0a0[7]==0x2aaaU && *range.cache_x_at_3aad0c0==-1.0F && *range.cache_y_at_3aad0c4==-60000.0F,"781570 block");
        device_state.viewport_at_3d12780={0,0,640,480,bits(0.0F),bits(1.0F)};
        for(std::size_t i=0;i<16;++i)device_state.matrix_at_3d0fd80[i]=i%5==0?bits(1.0F):0U;
        NinjaRenderState render;render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});
        std::vector<std::pair<std::uint32_t,std::uint32_t>> calls;NinjaScreenDevice device;
        device.set_transform=[&](const D3D8FrameCommand& c,std::uint32_t s,const D3dxMatrix&)->std::optional<std::int32_t>{calls.push_back({c.source_call,s});return 0;};
        device.multiply_transform=[&](const D3D8FrameCommand& c,std::uint32_t s,const D3dxMatrix&)->std::optional<std::int32_t>{calls.push_back({c.source_call,s});return 0;};
        NinjaMatrix filler;filler.fill(7.0F);NinjaMatrixStack stack(std::vector<NinjaMatrix>(3,filler),3,0);
        X87Context fp(0x27f);SseContext sse(0x1f80);
        NinjaViewRuntime rt{view,range,projection,screen,device_state,render,device,stack,table,fp,sse,sse_reciprocal_by_division};
        const auto r=ninja_set_view_781250(rt);
        require(r && r.inverse_stored,"781250 completes");
        // Direction (0,0,-1), roll 0: the view is diag(-1, 1, -1) (pitch 0, yaw from z) at the origin.
        require(fl(*view.view_at_3d0f760[0])==-1.0F && fl(*view.view_at_3d0f760[5])==1.0F && fl(*view.view_at_3d0f760[10])==-1.0F && fl(*view.view_at_3d0f760[15])==1.0F,"default view");
        require(stack.depth()==1 && stack.current()[0]==1.0F && stack.current()[5]==-1.0F && stack.current()[10]==-1.0F,"slot 0 negates columns 0 and 1");
        require(*view.view_copy_at_3d0f980[0]==bits(1.0F),"3d0f980 copy");
        require(*projection.scale_at_3d0f9e0>554.0F && *projection.scale_at_3d0f9e0<555.0F,"640 / (2 tan 30deg)");
        require(*range.near_at_3d0f808==1.0F && *range.far_at_3d0f968==60000.0F,"near/far negated");
        bool nonzero=false;for(const auto& w:screen.projection_at_3d129c0)nonzero=nonzero||(w&&*w!=0);
        require(nonzero && fl(*screen.projection_at_3d129c0[5])<0.0F && fl(*screen.projection_at_3d129c0[11])==1.0F,"3d129c0 = diag(1,-1,1,1) x perspective");
        require(calls.size()==3 && calls[0]==std::make_pair(0x79127eU,3U) && calls[1]==std::make_pair(0x791293U,3U) && calls[2]==std::make_pair(0x7912aaU,2U),"device calls");
        require(*view.view_at_389d398[0]==bits(1.0F) && fl(*view.view_inverse_at_389d358[5])==-1.0F,"389d398 copy and 389d358 inverse");
        // A missing viewport height stops at its FILD with the fact named; the view part has already run.
        device_state.viewport_at_3d12780[3].reset();
        const auto missing=ninja_set_view_781250(rt);
        require(missing.failure==NinjaViewFailure::unknown_state && missing.source_boundary==0x79118a && missing.required_fact==0x3d1278cU,"missing height");
        // No device: SetTransform's device load is the boundary.
        device_state.viewport_at_3d12780[3]=480;render.device_at_3d128b0.reset();
        const auto no_device=ninja_set_view_781250(rt);
        require(no_device.failure==NinjaViewFailure::unknown_device && no_device.source_boundary==0x79126f,"unknown device");
        std::cout<<"Ninja view: 781570 block, default view, stack reset, projection scale, 3d129c0, device calls, inverse and failures passed.\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
