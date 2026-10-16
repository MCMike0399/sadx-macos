#include "guest/frame_control_reference.hpp"
#include "engine/ninja_screen.hpp"
#include <cstring>
#include <iostream>
// 780c30/7912e0/78dbe0/77dfc0: the 640x480 screen, a clean viewport flag, the projected depth,
// the drawn TRIANGLEFAN and the unknown-state, null-device stops.
namespace {using namespace sadx;void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::uint32_t b(float f){std::uint32_t v;std::memcpy(&v,&f,4);return v;}}
int main(){try{
    NinjaScreenState s;NinjaDeviceState d;s.invalid_at_3d0fd1c=0;
    const std::uint32_t viewport[6]={0,0,640,480,0,b(1)};for(int i=0;i<6;++i)d.viewport_at_3d12780[i]=viewport[i];
    for(int i=0;i<16;++i){d.matrix_at_389d318[i]=i%5==0?b(1):0u;s.projection_at_3d129c0[i]=i%5==0?b(1):0u;}
    NinjaRenderState render;render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});
    const auto blend=decode_ninja_blend_tables(std::vector<std::uint8_t>(56,0).data(),56);render.blend_tables=&*blend;render.packed_blend_at_3ab9890=0;
    std::size_t states=0,stages=0;render.set_render_state=[&](const NinjaRenderStateCommand&){++states;return NinjaRenderCallResult{true,0};};
    render.set_texture_stage_state=[&](const NinjaRenderStageCommand&){++stages;return NinjaRenderCallResult{true,0};};
    NinjaScreenDevice device;std::optional<NinjaScreenPolygon> drawn;
    device.set_vertex_shader=[](const D3D8FrameCommand&,std::uint32_t)->std::optional<std::int32_t>{return 0;};
    device.draw_primitive_up=[&](const D3D8FrameCommand&,const NinjaScreenPolygon& p)->std::optional<std::int32_t>{drawn=p;return 0;};
    GameFrameRendererState renderer{};renderer.counter_frequency.low=3579545;renderer.counter_frequency.high=0;
    std::optional<float> width;NinjaSpriteProjection projection;NinjaCallbackList callbacks;callbacks.count_at_3ce712c=3;
    std::optional<std::uint32_t> w840,w9c4,pool=9,size=9;std::array<std::optional<std::uint32_t>,4> colors{7u,7u,7u,7u};
    X87Context fp(0x27f);SseContext sse(0x1f80);
    NinjaScreenInitRuntime init{NinjaViewportRuntime{s,d,render,device,fp},renderer,width,projection,callbacks,w840,w9c4,pool,size,colors};
    require(bool(run_screen_init_780c30(0x30,0,1,init)),"screen init");
    require(width==640.0F&&s.height_at_3d0f9e8==480.0F&&s.right_at_3d0f828==639.0F&&projection.center_x_at_3d0fa0c==320.0F&&callbacks.count_at_3ce712c==0u&&w840==0xffffffffu&&colors[0]==0u,"640x480 screen");
    require(s.points_at_3ce7184[4]==b(640)&&s.points_at_3ce7184[5]==b(480)&&s.count_at_3ce7170==4u&&s.invalid_at_3d0fd1c==0u,"polygon block and a clean viewport flag");
    NinjaDepthRuntime depth{s,d,sse,fp,sse_reciprocal_by_division};
    const auto z=ninja_projected_depth_78dbe0(1.0F,depth);require(z&&fp.to_float(*z.value)==1.0F,"identity projection keeps z = 1/w");
    NinjaTextureSelectionState textures;textures.global_id_at_3d0fdf4=0xffffffffU;std::size_t binds=0;
    NinjaTextureSelectionRuntime texture_runtime{render,nullptr,nullptr,[&](const NinjaTextureBindCommand&){++binds;return NinjaRenderCallResult{true,0};}};
    const std::optional<std::uint32_t> buffer=0x1000;
    NinjaPolygonRuntime polygon{[&](std::uint32_t a)->std::optional<std::uint32_t>{
            if(a==0x3ce7164)return s.points_at_3ce7164;if(a==0x3ce7168)return s.colors_at_3ce7168;
            if(a>=0x3ce7184&&a<0x3ce71a4)return s.points_at_3ce7184[(a-0x3ce7184)/4];if(a>=0x3ce7174&&a<0x3ce7184)return colors[(a-0x3ce7174)/4];return std::nullopt;},
        render,textures,texture_runtime,buffer,device,depth};
    const auto r=run_screen_polygon_77dfc0(0x3ce7164,4,-1.0F,0x22,polygon);
    require(r&&drawn&&drawn->primitive_count==2&&drawn->vertex_bytes.size()==80&&binds==0,"four-vertex fan");
    std::uint32_t w[5];std::memcpy(w,drawn->vertex_bytes.data()+40,20);require(w[0]==b(640.5F)&&w[1]==b(480.5F)&&w[2]==b(1)&&w[3]==b(1),"half-pixel offset and depth");
    textures.global_id_at_3d0fdf4.reset();require(run_screen_polygon_77dfc0(0x3ce7164,4,-1.0F,0x22,polygon).status.required_fact==0x3d0fdf4u,"unknown texture id");
    textures.global_id_at_3d0fdf4=1;render.device_at_3d128b0=NinjaRenderDeviceHandle{};
    require(run_screen_polygon_77dfc0(0x3ce7164,4,-1.0F,0x22,polygon).status.failure==NinjaScreenFailure::null_device,"null device");
    require(bool(run_screen_polygon_77dfc0(0x3ce7164,2,-1.0F,0x22,polygon)),"two vertices draw nothing");
    render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});
    s.invalid_at_3d0fd1c=0;s.points_at_3ce7184[2]=0xfeedbeefU;
    init.final_inputs_ready=[&](){s.width_at_3d0fa04.reset();return true;};
    const auto absent_extent=run_screen_init_780c30(0x30,0,1,init);
    require(!absent_extent&&absent_extent.source_boundary==0x780dee&&absent_extent.required_fact==0x3d0fa04U
        &&s.points_at_3ce7184[2]==0xfeedbeefU,"missing final extent stops before polygon stores");
    bool multiplied=false;s.invalid_at_3d0fd1c=1;
    device.set_viewport=[](const D3D8FrameCommand&,const std::array<std::uint32_t,6>&)->std::optional<std::int32_t>{return 0;};
    device.set_transform=[&](const D3D8FrameCommand&,std::uint32_t,const D3dxMatrix&)->std::optional<std::int32_t>{d.matrix_at_3d0fd80[0].reset();return 0;};
    device.multiply_transform=[&](const D3D8FrameCommand&,std::uint32_t,const D3dxMatrix&)->std::optional<std::int32_t>{multiplied=true;return 0;};
    NinjaViewportRuntime viewport_runtime{s,d,render,device,fp};const auto absent_matrix=run_viewport_refresh_7912e0(viewport_runtime);
    require(!absent_matrix&&absent_matrix.source_boundary==0x7913c2&&absent_matrix.required_fact==0x3d0fd80U&&!multiplied,
        "callback-unavailable matrix stops explicitly before multiply");
    std::cout<<"Screen 780c30/7912e0/78dbe0/77dfc0: 640x480 setup, depth, fan and stops passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
