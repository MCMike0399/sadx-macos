// Direct3D 8 for the guest: Direct3DCreate8, IDirect3D8 and IDirect3DDevice8 as COM objects whose methods
// are HLE traps (vtable order and stdcall sizes from d3d8.h). Device state the source sets is recorded in a
// D3D8StateOwner (HleProcess::d3d8), the same recording owner the native runtime uses, so a Metal backend can
// read either. Resources (textures, vertex/index buffers, surfaces) are added as the guest reaches them;
// every method without a body stops the guest with its name.
#include "hle/hle_com.hpp"
#include <cstring>
#include <memory>
namespace sadx {
namespace {
// (name, argument bytes including `this`) in vtable order.
struct Slot {const char* name;std::uint32_t bytes;};
constexpr Slot direct3d8[]={{"QueryInterface",12},{"AddRef",4},{"Release",4},{"RegisterSoftwareDevice",8},{"GetAdapterCount",4},{"GetAdapterIdentifier",16},
    {"GetAdapterModeCount",8},{"EnumAdapterModes",16},{"GetAdapterDisplayMode",12},{"CheckDeviceType",24},{"CheckDeviceFormat",28},{"CheckDeviceMultiSampleType",24},
    {"CheckDepthStencilMatch",24},{"GetDeviceCaps",16},{"GetAdapterMonitor",8},{"CreateDevice",28}};
constexpr Slot device8[]={{"QueryInterface",12},{"AddRef",4},{"Release",4},{"TestCooperativeLevel",4},{"GetAvailableTextureMem",4},{"ResourceManagerDiscardBytes",8},
    {"GetDirect3D",8},{"GetDeviceCaps",8},{"GetDisplayMode",8},{"GetCreationParameters",8},{"SetCursorProperties",16},{"SetCursorPosition",16},{"ShowCursor",8},
    {"CreateAdditionalSwapChain",12},{"Reset",8},{"Present",20},{"GetBackBuffer",16},{"GetRasterStatus",8},{"SetGammaRamp",12},{"GetGammaRamp",8},
    {"CreateTexture",32},{"CreateVolumeTexture",36},{"CreateCubeTexture",28},{"CreateVertexBuffer",24},{"CreateIndexBuffer",24},{"CreateRenderTarget",28},
    {"CreateDepthStencilSurface",24},{"CreateImageSurface",20},{"CopyRects",24},{"UpdateTexture",12},{"GetFrontBuffer",8},{"SetRenderTarget",12},
    {"GetRenderTarget",8},{"GetDepthStencilSurface",8},{"BeginScene",4},{"EndScene",4},{"Clear",28},{"SetTransform",12},{"GetTransform",12},
    {"MultiplyTransform",12},{"SetViewport",8},{"GetViewport",8},{"SetMaterial",8},{"GetMaterial",8},{"SetLight",12},{"GetLight",12},{"LightEnable",12},
    {"GetLightEnable",12},{"SetClipPlane",12},{"GetClipPlane",12},{"SetRenderState",12},{"GetRenderState",12},{"BeginStateBlock",4},{"EndStateBlock",8},
    {"ApplyStateBlock",8},{"CaptureStateBlock",8},{"DeleteStateBlock",8},{"CreateStateBlock",12},{"SetClipStatus",8},{"GetClipStatus",8},{"GetTexture",12},
    {"SetTexture",12},{"GetTextureStageState",16},{"SetTextureStageState",16},{"ValidateDevice",8},{"GetInfo",16},{"SetPaletteEntries",12},
    {"GetPaletteEntries",12},{"SetCurrentTexturePalette",8},{"GetCurrentTexturePalette",8},{"DrawPrimitive",16},{"DrawIndexedPrimitive",24},
    {"DrawPrimitiveUP",20},{"DrawIndexedPrimitiveUP",36},{"ProcessVertices",24},{"CreateVertexShader",20},{"SetVertexShader",8},{"GetVertexShader",8},
    {"DeleteVertexShader",8},{"SetVertexShaderConstant",16},{"GetVertexShaderConstant",16},{"GetVertexShaderDeclaration",16},{"GetVertexShaderFunction",16},
    {"SetStreamSource",16},{"GetStreamSource",16},{"SetIndices",12},{"GetIndices",12},{"CreatePixelShader",12},{"SetPixelShader",8},{"GetPixelShader",8},
    {"DeletePixelShader",8},{"SetPixelShaderConstant",16},{"GetPixelShaderConstant",16},{"GetPixelShaderFunction",16},{"DrawRectPatch",16},
    {"DrawTriPatch",16},{"DeletePatch",8}};
std::vector<HleComMethod> methods(const char* interface_name,const Slot* slots,std::size_t count,const std::map<std::string,std::function<std::uint32_t(HleCall&)>>& bodies){
    std::vector<HleComMethod> out;
    for(std::size_t i=0;i<count;++i){
        const std::string name=slots[i].name;const auto found=bodies.find(name);
        if(found!=bodies.end()){out.push_back({name,slots[i].bytes,found->second});continue;}
        const std::string full=std::string(interface_name)+"::"+name;
        out.push_back({name,slots[i].bytes,[full](HleCall& c)->std::uint32_t{c.guest.stop(full+" is not modelled yet");return 0x8876086cU;}});} // D3DERR_INVALIDCALL
    return out;}
constexpr std::uint32_t D3D_OK=0,adapter_format=22; // D3DFMT_X8R8G8B8
constexpr Slot texture8[]={{"QueryInterface",12},{"AddRef",4},{"Release",4},{"GetDevice",8},{"SetPrivateData",20},{"GetPrivateData",16},{"FreePrivateData",8},
    {"SetPriority",8},{"GetPriority",4},{"PreLoad",4},{"GetType",4},{"SetLOD",8},{"GetLOD",4},{"GetLevelCount",4},{"GetLevelDesc",12},{"GetSurfaceLevel",12},
    {"LockRect",20},{"UnlockRect",8},{"AddDirtyRect",8}};
constexpr Slot surface8[]={{"QueryInterface",12},{"AddRef",4},{"Release",4},{"GetDevice",8},{"SetPrivateData",20},{"GetPrivateData",16},{"FreePrivateData",8},
    {"GetContainer",12},{"GetDesc",8},{"LockRect",16},{"UnlockRect",4}};
constexpr Slot buffer8[]={{"QueryInterface",12},{"AddRef",4},{"Release",4},{"GetDevice",8},{"SetPrivateData",20},{"GetPrivateData",16},{"FreePrivateData",8},
    {"SetPriority",8},{"GetPriority",4},{"PreLoad",4},{"GetType",4},{"Lock",20},{"Unlock",4},{"GetDesc",8}};
// Bytes per row of `width` pixels (DXT: per row of 4x4 blocks) and the number of rows.
std::uint32_t pitch_of(std::uint32_t format,std::uint32_t width){
    const auto fourcc=[](const char* s){return static_cast<std::uint32_t>(s[0])|(static_cast<std::uint32_t>(s[1])<<8)|(static_cast<std::uint32_t>(s[2])<<16)|(static_cast<std::uint32_t>(s[3])<<24);};
    if(format==fourcc("DXT1"))return std::max(1U,(width+3)/4)*8;
    if(format==fourcc("DXT2")||format==fourcc("DXT3")||format==fourcc("DXT4")||format==fourcc("DXT5"))return std::max(1U,(width+3)/4)*16;
    switch(format){case 20:case 21:case 22:case 71:case 75:case 77:return width*4;case 23:case 24:case 25:case 26:case 80:return width*2;case 28:case 50:case 41:return width;default:return 0;}}
std::uint32_t rows_of(std::uint32_t format,std::uint32_t height){
    const bool dxt=format>0xffff;return dxt?std::max(1U,(height+3)/4):height;}
// D3DCAPS8 (0xd4 bytes) for IDirect3D8::GetDeviceCaps and IDirect3DDevice8::GetDeviceCaps. sonic.exe reads three
// fields of its copy at 3d127c0: TextureCaps (+0x3c) bit MIPMAP (78cfa0: without it the mipmapped-PVR handlers are
// cleared and the title screen's twiddled-mipmap textures reach the fatal 789dd0) and bit CUBEMAP (7914b0, 78db80),
// and RasterCaps (+0x24) bit WFOG (794edb). Declared: a HAL device with the texture caps every D3D8-class GPU has.
std::vector<std::uint8_t> device_caps(HleProcess& p){
    p.assume("D3D8 GetDeviceCaps: a HAL device with TextureCaps PERSPECTIVE|ALPHA|PROJECTED|CUBEMAP|VOLUMEMAP|MIPMAP|MIPVOLUMEMAP|MIPCUBEMAP and RasterCaps WFOG, every other cap 0 (the reference GPU's D3DCAPS8 were not captured; the native runtime still declares only WFOG)");
    std::vector<std::uint8_t> caps(0xd4,0);
    const auto put=[&](std::size_t at,std::uint32_t v){for(int k=0;k<4;++k)caps[at+k]=static_cast<std::uint8_t>(v>>(8*k));};
    put(0x00,1);          // DeviceType = D3DDEVTYPE_HAL
    put(0x24,0x100000);   // RasterCaps = D3DPRASTERCAPS_WFOG
    put(0x3c,0x1ec05);    // TextureCaps = PERSPECTIVE 1 | ALPHA 4 | PROJECTED 400 | CUBEMAP 800 | VOLUMEMAP 2000 | MIPMAP 4000 | MIPVOLUMEMAP 8000 | MIPCUBEMAP 10000
    return caps;}
}
void register_hle_d3d8(HleProcess& p){
    auto* P=&p;
    // Resource objects. Object words: +0 vtable, +4 references, +8 width, +12 height, +16 format, +20 pitch,
    // +24 bits (guest memory), +28 size, +32 container (surfaces: the texture), +36 levels / usage, +40 level
    // table (textures: surface objects), +44 type tag.
    auto surface_if=std::make_shared<std::unique_ptr<HleComInterface>>();auto texture_if=std::make_shared<std::unique_ptr<HleComInterface>>();
    auto buffer_if=std::make_shared<std::unique_ptr<HleComInterface>>();
    const auto arg=[](HleCall& c,unsigned i){return c.argument(i).value_or(0);};
    const auto field=[](X86Guest& g,std::uint32_t o,std::uint32_t at){return g.u32(o+at).value_or(0);};
    const auto surface_interface=[P,surface_if,arg,field]()->HleComInterface&{
        if(*surface_if)return **surface_if;
        std::map<std::string,std::function<std::uint32_t(HleCall&)>> b;
        b["AddRef"]=com_add_ref().body;b["Release"]=com_release().body;
        b["GetDesc"]=[arg,field](HleCall& c)->std::uint32_t{ // D3DSURFACE_DESC: Format, Type(1 surface), Usage, Pool, Size, MultiSampleType, Width, Height
            const auto o=arg(c,0);const std::uint32_t d[8]={field(c.guest,o,16),1,field(c.guest,o,36),0,field(c.guest,o,28),0,field(c.guest,o,8),field(c.guest,o,12)};c.guest.write(arg(c,1),d,32);return D3D_OK;};
        b["LockRect"]=[arg,field](HleCall& c)->std::uint32_t{
            const auto o=arg(c,0);if(arg(c,2)){c.guest.stop("IDirect3DSurface8::LockRect of a sub-rectangle is not modelled yet");return 0;}
            const std::uint32_t locked[2]={field(c.guest,o,20),field(c.guest,o,24)};c.guest.write(arg(c,1),locked,8);return D3D_OK;};
        b["UnlockRect"]=[](HleCall&)->std::uint32_t{return D3D_OK;};
        b["GetContainer"]=[arg,field](HleCall& c)->std::uint32_t{const auto t=field(c.guest,arg(c,0),32);if(!t)return 0x8876086cU;c.guest.set_u32(t+4,c.guest.u32(t+4).value_or(0)+1);c.guest.set_u32(arg(c,2),t);return D3D_OK;};
        *surface_if=std::make_unique<HleComInterface>(*P,"IDirect3DSurface8",methods("IDirect3DSurface8",surface8,std::size(surface8),b));(void)P;return **surface_if;};
    const auto new_surface=[P,surface_interface](X86Guest& g,std::uint32_t width,std::uint32_t height,std::uint32_t format,std::uint32_t usage,std::uint32_t container)->std::optional<std::uint32_t>{
        const auto o=surface_interface().create(48);if(!o)return std::nullopt;const auto pitch=pitch_of(format,width);const auto size=pitch*rows_of(format,height);
        if(!pitch){g.stop("D3D8 surface format "+std::to_string(format)+" has no modelled pitch");return std::nullopt;}
        const auto bits=P->memory().allocate(std::max(size,4U),16);if(!bits)return std::nullopt;
        const std::uint32_t w[10]={width,height,format,pitch,*bits,size,container,usage,0,0x53555246U};g.write(*o+8,w,40);return o;};
    const auto texture_interface=[P,texture_if,arg,field]()->HleComInterface&{
        if(*texture_if)return **texture_if;
        std::map<std::string,std::function<std::uint32_t(HleCall&)>> b;
        b["AddRef"]=com_add_ref().body;b["Release"]=com_release().body;
        b["GetLevelCount"]=[arg,field](HleCall& c)->std::uint32_t{return field(c.guest,arg(c,0),36);};
        b["GetType"]=[](HleCall&)->std::uint32_t{return 3;}; // D3DRTYPE_TEXTURE
        b["SetPriority"]=[](HleCall&)->std::uint32_t{return 0;};b["PreLoad"]=[](HleCall&)->std::uint32_t{return 0;};
        const auto level=[field](X86Guest& g,std::uint32_t o,std::uint32_t i)->std::uint32_t{if(i>=field(g,o,36))return 0;return g.u32(field(g,o,40)+4*i).value_or(0);};
        b["GetLevelDesc"]=[arg,field,level](HleCall& c)->std::uint32_t{
            const auto s=level(c.guest,arg(c,0),arg(c,1));if(!s)return 0x8876086cU;
            const std::uint32_t d[8]={field(c.guest,s,16),1,field(c.guest,s,36),0,field(c.guest,s,28),0,field(c.guest,s,8),field(c.guest,s,12)};c.guest.write(arg(c,2),d,32);return D3D_OK;};
        b["GetSurfaceLevel"]=[arg,level](HleCall& c)->std::uint32_t{
            const auto s=level(c.guest,arg(c,0),arg(c,1));if(!s)return 0x8876086cU;c.guest.set_u32(s+4,c.guest.u32(s+4).value_or(0)+1);c.guest.set_u32(arg(c,2),s);return D3D_OK;};
        b["LockRect"]=[arg,field,level](HleCall& c)->std::uint32_t{
            const auto s=level(c.guest,arg(c,0),arg(c,1));if(!s)return 0x8876086cU;
            if(arg(c,3)){c.guest.stop("IDirect3DTexture8::LockRect of a sub-rectangle is not modelled yet");return 0;}
            const std::uint32_t locked[2]={field(c.guest,s,20),field(c.guest,s,24)};c.guest.write(arg(c,2),locked,8);return D3D_OK;};
        b["UnlockRect"]=[](HleCall&)->std::uint32_t{return D3D_OK;};
        b["AddDirtyRect"]=[](HleCall&)->std::uint32_t{return D3D_OK;};
        *texture_if=std::make_unique<HleComInterface>(*P,"IDirect3DTexture8",methods("IDirect3DTexture8",texture8,std::size(texture8),b));return **texture_if;};
    const auto buffer_interface=[P,buffer_if,arg,field]()->HleComInterface&{
        if(*buffer_if)return **buffer_if;
        std::map<std::string,std::function<std::uint32_t(HleCall&)>> b;
        b["AddRef"]=com_add_ref().body;b["Release"]=com_release().body;
        b["Lock"]=[arg,field](HleCall& c)->std::uint32_t{
            const auto o=arg(c,0);const auto offset=arg(c,1);if(offset>field(c.guest,o,28))return 0x8876086cU;
            c.guest.set_u32(arg(c,3),field(c.guest,o,24)+offset);return D3D_OK;};
        b["Unlock"]=[](HleCall&)->std::uint32_t{return D3D_OK;};
        b["GetDesc"]=[arg,field](HleCall& c)->std::uint32_t{ // D3DVERTEXBUFFER_DESC / D3DINDEXBUFFER_DESC: Format, Type, Usage, Pool, Size, FVF
            const auto o=arg(c,0);const std::uint32_t d[6]={field(c.guest,o,16),field(c.guest,o,44),field(c.guest,o,36),field(c.guest,o,20),field(c.guest,o,28),field(c.guest,o,12)};c.guest.write(arg(c,1),d,24);return D3D_OK;};
        b["SetPriority"]=[](HleCall&)->std::uint32_t{return 0;};b["PreLoad"]=[](HleCall&)->std::uint32_t{return 0;};
        *buffer_if=std::make_unique<HleComInterface>(*P,"IDirect3DBuffer8",methods("IDirect3DBuffer8",buffer8,std::size(buffer8),b));return **buffer_if;};
    auto device_interface=std::make_shared<std::unique_ptr<HleComInterface>>();
    auto d3d_interface=std::make_shared<std::unique_ptr<HleComInterface>>();
    auto d3d_object=std::make_shared<std::uint32_t>(0);
    // Device-held objects: the back buffer and depth surface, bound textures/streams/indices, lights, materials.
    struct Bound {std::uint32_t back_buffer=0,depth=0,render_target=0,indices=0,base_vertex=0;std::map<std::uint32_t,std::uint32_t> textures;
        std::map<std::uint32_t,std::pair<std::uint32_t,std::uint32_t>> streams;std::map<std::uint32_t,std::vector<std::uint8_t>> lights,clip_planes;std::map<std::uint32_t,std::uint32_t> light_enabled;
        std::vector<std::uint8_t> material;};
    auto bound=std::make_shared<Bound>();
    const auto display_mode=[P](HleCall& c,std::uint32_t at){
        P->assume("the D3D8 adapter is the 1920x1080 desktop at 60 Hz in D3DFMT_X8R8G8B8 (the reference GPU and display mode were not captured)");
        const std::uint32_t mode[4]={1920,1080,60,adapter_format};c.guest.write(at,mode,16);};
    const auto device=[P,device_interface,display_mode,arg,d3d_object,bound,new_surface,texture_interface,buffer_interface,field]()->HleComInterface&{
        if(*device_interface)return **device_interface;
        auto& d3d8=P->d3d8;
        std::map<std::string,std::function<std::uint32_t(HleCall&)>> b;
        b["AddRef"]=com_add_ref().body;b["Release"]=com_release().body;
        b["GetDirect3D"]=[d3d_object,arg](HleCall& c)->std::uint32_t{c.guest.set_u32(arg(c,1),*d3d_object);return D3D_OK;};
        b["GetDisplayMode"]=[display_mode,arg](HleCall& c)->std::uint32_t{display_mode(c,arg(c,1));return D3D_OK;};
        b["TestCooperativeLevel"]=[](HleCall&)->std::uint32_t{return D3D_OK;};
        b["BeginScene"]=[&d3d8](HleCall&)->std::uint32_t{++d3d8.begin_scenes;return D3D_OK;};
        b["EndScene"]=[&d3d8](HleCall&)->std::uint32_t{++d3d8.end_scenes;return D3D_OK;};
        b["Clear"]=[P,&d3d8,arg](HleCall& c)->std::uint32_t{++d3d8.clears;d3d8.last_clear_flags=arg(c,3);
            if(arg(c,1)){c.guest.stop("IDirect3DDevice8::Clear of sub-rectangles is not modelled");return 0;}
            P->frame.clears.push_back({arg(c,3),arg(c,4),arg(c,5),arg(c,6)});return D3D_OK;}; // flags, colour, Z (float bits), stencil
        b["Present"]=[P,&d3d8](HleCall& c)->std::uint32_t{++d3d8.presents;P->frame.viewport=d3d8.viewport;P->last_frame=std::move(P->frame);P->frame={};
            if(P->on_present)P->on_present(c.guest);return D3D_OK;};
        b["SetRenderState"]=[&d3d8,arg](HleCall& c)->std::uint32_t{d3d8.render_states[arg(c,1)]=arg(c,2);++d3d8.render_calls;return D3D_OK;};
        b["GetRenderState"]=[&d3d8,arg](HleCall& c)->std::uint32_t{
            const auto i=d3d8.render_states.find(arg(c,1));if(i==d3d8.render_states.end()){c.guest.stop("GetRenderState("+std::to_string(arg(c,1))+") before any SetRenderState: the device's default is not modelled");return 0;}
            c.guest.set_u32(arg(c,2),i->second);return D3D_OK;};
        b["SetTextureStageState"]=[&d3d8,arg](HleCall& c)->std::uint32_t{if(arg(c,1)==0)d3d8.stage0_states[arg(c,2)]=arg(c,3);++d3d8.stage_calls;return D3D_OK;};
        const auto set_matrix=[&d3d8,arg](HleCall& c)->std::uint32_t{NinjaMatrix m{};c.guest.read(arg(c,2),m.data(),64);d3d8.transforms[arg(c,1)]=m;return D3D_OK;};
        b["SetTransform"]=set_matrix;
        b["MultiplyTransform"]=[P,&d3d8,arg](HleCall& c)->std::uint32_t{
            P->assume("IDirect3DDevice8::MultiplyTransform computes pMatrix x current in single precision, row by column in index order (the D3D runtime's own rounding was not captured)");
            NinjaMatrix m{};c.guest.read(arg(c,2),m.data(),64);NinjaMatrix current{};
            if(const auto i=d3d8.transforms.find(arg(c,1));i!=d3d8.transforms.end())current=i->second;else current={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
            NinjaMatrix out{};for(int r=0;r<4;++r)for(int k=0;k<4;++k){float sum=0;for(int j=0;j<4;++j)sum+=m[r*4+j]*current[j*4+k];out[r*4+k]=sum;}
            d3d8.transforms[arg(c,1)]=out;return D3D_OK;};
        b["GetTransform"]=[&d3d8,arg](HleCall& c)->std::uint32_t{
            const auto i=d3d8.transforms.find(arg(c,1));
            if(i==d3d8.transforms.end()){const float id[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};c.guest.write(arg(c,2),id,64);return D3D_OK;} // D3D8's default: identity
            c.guest.write(arg(c,2),i->second.data(),64);return D3D_OK;};
        b["SetViewport"]=[&d3d8,arg](HleCall& c)->std::uint32_t{std::array<std::uint32_t,6> v{};c.guest.read(arg(c,1),v.data(),24);d3d8.viewport=v;return D3D_OK;};
        b["GetViewport"]=[&d3d8,arg,bound,field](HleCall& c)->std::uint32_t{
            if(!d3d8.viewport){ // D3D8's default: the whole render target, Z 0..1
                const auto t=bound->back_buffer;const std::uint32_t v[6]={0,0,field(c.guest,t,8),field(c.guest,t,12),0,0x3f800000U};c.guest.write(arg(c,1),v,24);return D3D_OK;}
            c.guest.write(arg(c,1),d3d8.viewport->data(),24);return D3D_OK;};
        b["SetVertexShader"]=[P,&d3d8,arg](HleCall& c)->std::uint32_t{++d3d8.vertex_formats;P->vertex_shader=arg(c,1);return D3D_OK;}; // D3D8: an FVF code is a shader handle
        b["SetTexture"]=[&d3d8](HleCall&)->std::uint32_t{++d3d8.texture_binds;return D3D_OK;};
        b["ShowCursor"]=[](HleCall&)->std::uint32_t{return 1;};
        const auto add_ref=[](X86Guest& g,std::uint32_t o){if(o)g.set_u32(o+4,g.u32(o+4).value_or(0)+1);};
        const auto release=[](X86Guest& g,std::uint32_t o){if(o){const auto n=g.u32(o+4).value_or(1);g.set_u32(o+4,n?n-1:0);}};
        b["GetAvailableTextureMem"]=[P](HleCall&)->std::uint32_t{P->assume("D3D8 GetAvailableTextureMem reports 512 MiB (the reference GPU was not captured)");return 0x20000000U;};
        b["ResourceManagerDiscardBytes"]=[](HleCall&)->std::uint32_t{return D3D_OK;};
        b["SetLight"]=[bound,arg](HleCall& c)->std::uint32_t{std::vector<std::uint8_t> l(104);c.guest.read(arg(c,2),l.data(),104);bound->lights[arg(c,1)]=l;return D3D_OK;};
        b["GetLight"]=[bound,arg](HleCall& c)->std::uint32_t{const auto i=bound->lights.find(arg(c,1));if(i==bound->lights.end())return 0x8876086cU;c.guest.write(arg(c,2),i->second.data(),104);return D3D_OK;};
        b["LightEnable"]=[bound,arg](HleCall& c)->std::uint32_t{bound->light_enabled[arg(c,1)]=arg(c,2)?1:0;if(!bound->lights.count(arg(c,1)))bound->lights[arg(c,1)]=std::vector<std::uint8_t>(104,0);return D3D_OK;};
        b["GetLightEnable"]=[bound,arg](HleCall& c)->std::uint32_t{const auto i=bound->light_enabled.find(arg(c,1));if(i==bound->light_enabled.end())return 0x8876086cU;c.guest.set_u32(arg(c,2),i->second);return D3D_OK;};
        b["SetMaterial"]=[bound,arg](HleCall& c)->std::uint32_t{bound->material.resize(68);c.guest.read(arg(c,1),bound->material.data(),68);return D3D_OK;};
        b["GetMaterial"]=[bound,arg](HleCall& c)->std::uint32_t{if(bound->material.empty()){c.guest.stop("GetMaterial before SetMaterial: the default material is not modelled");return 0;}c.guest.write(arg(c,1),bound->material.data(),68);return D3D_OK;};
        b["SetClipPlane"]=[bound,arg](HleCall& c)->std::uint32_t{std::vector<std::uint8_t> v(16);c.guest.read(arg(c,2),v.data(),16);bound->clip_planes[arg(c,1)]=v;return D3D_OK;};
        b["SetGammaRamp"]=[](HleCall&)->std::uint32_t{return 0;};
        b["SetPaletteEntries"]=[](HleCall&)->std::uint32_t{return D3D_OK;};b["SetCurrentTexturePalette"]=[](HleCall&)->std::uint32_t{return D3D_OK;};
        b["SetTexture"]=[&d3d8,bound,arg,add_ref,release](HleCall& c)->std::uint32_t{
            auto& slot=bound->textures[arg(c,1)];if(slot!=arg(c,2)){add_ref(c.guest,arg(c,2));release(c.guest,slot);slot=arg(c,2);}++d3d8.texture_binds;return D3D_OK;};
        b["GetTexture"]=[bound,arg,add_ref](HleCall& c)->std::uint32_t{const auto t=bound->textures[arg(c,1)];add_ref(c.guest,t);c.guest.set_u32(arg(c,2),t);return D3D_OK;};
        b["SetStreamSource"]=[bound,arg,add_ref,release](HleCall& c)->std::uint32_t{
            auto& slot=bound->streams[arg(c,1)];if(slot.first!=arg(c,2)){add_ref(c.guest,arg(c,2));release(c.guest,slot.first);}slot={arg(c,2),arg(c,3)};return D3D_OK;};
        // GetStreamSource(n, &buffer, &stride): AddRef'd, as the runtime does.
        b["GetStreamSource"]=[bound,arg,add_ref](HleCall& c)->std::uint32_t{const auto slot=bound->streams[arg(c,1)];add_ref(c.guest,slot.first);c.guest.set_u32(arg(c,2),slot.first);c.guest.set_u32(arg(c,3),slot.second);return D3D_OK;};
        b["GetIndices"]=[bound,arg,add_ref](HleCall& c)->std::uint32_t{add_ref(c.guest,bound->indices);c.guest.set_u32(arg(c,1),bound->indices);c.guest.set_u32(arg(c,2),bound->base_vertex);return D3D_OK;};
        b["SetIndices"]=[bound,arg,add_ref,release](HleCall& c)->std::uint32_t{if(bound->indices!=arg(c,1)){add_ref(c.guest,arg(c,1));release(c.guest,bound->indices);}bound->indices=arg(c,1);bound->base_vertex=arg(c,2);return D3D_OK;};
        b["GetVertexShader"]=[P,arg](HleCall& c)->std::uint32_t{c.guest.set_u32(arg(c,1),P->vertex_shader);return D3D_OK;};
        b["GetPixelShader"]=[arg](HleCall& c)->std::uint32_t{c.guest.set_u32(arg(c,1),0);return D3D_OK;}; // only the fixed-function pipeline is ever set
        b["SetPixelShader"]=[](HleCall& c)->std::uint32_t{if(c.argument(1).value_or(0))c.guest.stop("IDirect3DDevice8::SetPixelShader with a shader is not modelled");return D3D_OK;};
        b["ValidateDevice"]=[arg](HleCall& c)->std::uint32_t{c.guest.set_u32(arg(c,1),1);return D3D_OK;};
        // Draws are recorded (counts; DrawPrimitiveUP bytes as polygons, the native owner's form).
        b["DrawPrimitive"]=[&d3d8](HleCall&)->std::uint32_t{++d3d8.render_calls;return D3D_OK;};
        b["DrawIndexedPrimitive"]=[&d3d8](HleCall&)->std::uint32_t{++d3d8.render_calls;return D3D_OK;};
        b["DrawPrimitiveUP"]=[P,&d3d8,arg,bound](HleCall& c)->std::uint32_t{
            const auto type=arg(c,1),count=arg(c,2),stride=arg(c,4);const auto vertices=type==6||type==5?count+2:type==4?count*3:type==3?count+1:type==2?count*2:count;
            NinjaScreenPolygon poly;poly.primitive_type=type;poly.primitive_count=count;poly.stride=stride;poly.vertex_bytes.resize(static_cast<std::size_t>(vertices)*stride);
            c.guest.read(arg(c,3),poly.vertex_bytes.data(),poly.vertex_bytes.size());
            HleProcess::D3D8Draw draw;draw.primitive_type=type;draw.primitive_count=count;draw.fvf=P->vertex_shader;draw.stride=stride;draw.vertices=poly.vertex_bytes;
            draw.texture=bound->textures.count(0)?bound->textures[0]:0;draw.render_states=d3d8.render_states;draw.stage0_states=d3d8.stage0_states;
            P->frame.draws.push_back(std::move(draw));
            d3d8.polygons.push_back(std::move(poly));return D3D_OK;};
        b["DrawIndexedPrimitiveUP"]=[&d3d8](HleCall&)->std::uint32_t{++d3d8.render_calls;return D3D_OK;};
        // Resources.
        b["CreateTexture"]=[P,texture_interface,new_surface,arg](HleCall& c)->std::uint32_t{
            auto width=arg(c,1),height=arg(c,2),levels=arg(c,3);const auto usage=arg(c,4),format=arg(c,5);
            if(!width||!height)return 0x8876086cU;
            if(!levels){levels=1;for(auto w=width,h=height;w>1||h>1;w=std::max(1U,w/2),h=std::max(1U,h/2))++levels;}
            const auto o=texture_interface().create(48);if(!o)return 0x8007000eU;
            const auto table=P->memory().allocate(4*levels).value_or(0);
            for(std::uint32_t i=0;i<levels;++i){const auto s=new_surface(c.guest,std::max(1U,width>>i),std::max(1U,height>>i),format,usage,*o);if(!s)return 0x8007000eU;c.guest.set_u32(table+4*i,*s);}
            const std::uint32_t w[10]={width,height,format,0,0,0,0,levels,table,0x54584554U};c.guest.write(*o+8,w,40);
            c.guest.set_u32(arg(c,7),*o);return D3D_OK;};
        const auto create_buffer=[P,buffer_interface,arg](bool index){return [P,buffer_interface,arg,index](HleCall& c)->std::uint32_t{
            const auto length=arg(c,1),usage=arg(c,2),format_or_fvf=arg(c,3),pool=arg(c,4);
            const auto o=buffer_interface().create(48);if(!o)return 0x8007000eU;const auto bits=P->memory().allocate(std::max(length,4U),16);if(!bits)return 0x8007000eU;
            // +8 unused, +12 FVF (vertex buffers), +16 format (index buffers: D3DFMT_INDEX16/32; vertex: D3DFMT_VERTEXDATA 100), +20 pool, +24 bits, +28 size, +36 usage, +44 type
            const std::uint32_t w[10]={0,index?0:format_or_fvf,index?format_or_fvf:100U,pool,*bits,length,0,usage,0,index?7U:6U};c.guest.write(*o+8,w,40);
            c.guest.set_u32(arg(c,5),*o);return D3D_OK;};};
        b["CreateVertexBuffer"]=create_buffer(false);b["CreateIndexBuffer"]=create_buffer(true);
        b["CreateImageSurface"]=[new_surface,arg](HleCall& c)->std::uint32_t{const auto s=new_surface(c.guest,arg(c,1),arg(c,2),arg(c,3),0,0);if(!s)return 0x8007000eU;c.guest.set_u32(arg(c,4),*s);return D3D_OK;};
        b["CreateDepthStencilSurface"]=[new_surface,arg](HleCall& c)->std::uint32_t{const auto s=new_surface(c.guest,arg(c,1),arg(c,2),arg(c,3),2,0);if(!s)return 0x8007000eU;c.guest.set_u32(arg(c,5),*s);return D3D_OK;};
        b["CreateRenderTarget"]=[new_surface,arg](HleCall& c)->std::uint32_t{const auto s=new_surface(c.guest,arg(c,1),arg(c,2),arg(c,3),1,0);if(!s)return 0x8007000eU;c.guest.set_u32(arg(c,6),*s);return D3D_OK;};
        b["GetBackBuffer"]=[bound,arg,add_ref](HleCall& c)->std::uint32_t{if(arg(c,1)||!bound->back_buffer)return 0x8876086cU;add_ref(c.guest,bound->back_buffer);c.guest.set_u32(arg(c,3),bound->back_buffer);return D3D_OK;};
        b["GetRenderTarget"]=[bound,arg,add_ref](HleCall& c)->std::uint32_t{const auto t=bound->render_target?bound->render_target:bound->back_buffer;add_ref(c.guest,t);c.guest.set_u32(arg(c,1),t);return D3D_OK;};
        b["GetDepthStencilSurface"]=[bound,arg,add_ref](HleCall& c)->std::uint32_t{if(!bound->depth)return 0x8876086cU;add_ref(c.guest,bound->depth);c.guest.set_u32(arg(c,1),bound->depth);return D3D_OK;};
        b["SetRenderTarget"]=[bound,arg](HleCall& c)->std::uint32_t{if(arg(c,1))bound->render_target=arg(c,1);if(arg(c,2))bound->depth=arg(c,2);return D3D_OK;};
        b["GetDeviceCaps"]=[P,arg](HleCall& c)->std::uint32_t{const auto caps=device_caps(*P);c.guest.write(arg(c,1),caps.data(),caps.size());return D3D_OK;};
        b["GetCreationParameters"]=[P,arg](HleCall& c)->std::uint32_t{c.guest.stop("IDirect3DDevice8::GetCreationParameters is not modelled yet");(void)P;(void)arg;return 0;};
        *device_interface=std::make_unique<HleComInterface>(*P,"IDirect3DDevice8",methods("IDirect3DDevice8",device8,std::size(device8),b));
        return **device_interface;};
    const auto direct3d=[P,d3d_interface,device,display_mode,arg,bound,new_surface]()->HleComInterface&{
        if(*d3d_interface)return **d3d_interface;
        std::map<std::string,std::function<std::uint32_t(HleCall&)>> b;
        b["AddRef"]=com_add_ref().body;b["Release"]=com_release().body;
        b["GetAdapterCount"]=[](HleCall&)->std::uint32_t{return 1;};
        b["GetAdapterDisplayMode"]=[display_mode,arg](HleCall& c)->std::uint32_t{if(arg(c,1))return 0x8876086cU;display_mode(c,arg(c,2));return D3D_OK;};
        b["CheckDeviceType"]=[](HleCall&)->std::uint32_t{return D3D_OK;};
        b["GetDeviceCaps"]=[P,arg](HleCall& c)->std::uint32_t{const auto caps=device_caps(*P);c.guest.write(arg(c,3),caps.data(),caps.size());return D3D_OK;};
        b["GetAdapterIdentifier"]=[P,arg](HleCall& c)->std::uint32_t{
            P->assume("D3D8 adapter identifier: driver \"sadx-hle\", description \"sadx-macos Metal\", vendor/device ids 0 (the reference GPU was not captured)");
            if(arg(c,1))return 0x8876086cU;std::vector<std::uint8_t> id(1024+8+16+16+4,0);
            std::memcpy(id.data(),"sadx-hle",8);std::memcpy(id.data()+512,"sadx-macos Metal",16);c.guest.write(arg(c,3),id.data(),id.size());return D3D_OK;};
        // The adapter's mode list: standard sizes up to the desktop's 1920x1080, 60 Hz, X8R8G8B8 then R5G6B5.
        static const std::uint32_t sizes[][2]={{640,480},{800,600},{1024,768},{1280,720},{1280,1024},{1600,900},{1920,1080}};
        constexpr std::uint32_t mode_count=2*std::size(sizes);
        b["GetAdapterModeCount"]=[P,arg](HleCall& c)->std::uint32_t{
            P->assume("D3D8 adapter modes: 640x480, 800x600, 1024x768, 1280x720, 1280x1024, 1600x900, 1920x1080 at 60 Hz in X8R8G8B8 and R5G6B5 (the reference monitor's mode list was not captured)");
            return arg(c,1)?0:mode_count;};
        b["EnumAdapterModes"]=[arg](HleCall& c)->std::uint32_t{
            const auto i=arg(c,2);if(arg(c,1)||i>=mode_count)return 0x8876086cU;
            const std::uint32_t mode[4]={sizes[i%std::size(sizes)][0],sizes[i%std::size(sizes)][1],60,i<std::size(sizes)?22U:23U};c.guest.write(arg(c,3),mode,16);return D3D_OK;};
        b["CheckDeviceFormat"]=[P,arg](HleCall& c)->std::uint32_t{
            P->assume("D3D8 CheckDeviceFormat: A8R8G8B8, X8R8G8B8, R5G6B5, X1R5G5B5, A1R5G5B5, A4R4G4B4, L8, A8 and DXT1-5 textures, D16/D24S8/D24X8/D32 depth are available, nothing else (a modern GPU; the reference GPU was not captured)");
            const auto usage=arg(c,4),format=arg(c,6);
            const bool depth=(usage&2)!=0; // D3DUSAGE_DEPTHSTENCIL
            const auto fourcc=[](const char* s){return static_cast<std::uint32_t>(s[0])|(static_cast<std::uint32_t>(s[1])<<8)|(static_cast<std::uint32_t>(s[2])<<16)|(static_cast<std::uint32_t>(s[3])<<24);};
            const bool ok=depth?(format==80||format==75||format==77||format==71)
                :(format==21||format==22||format==23||format==24||format==25||format==26||format==50||format==28||
                  format==fourcc("DXT1")||format==fourcc("DXT2")||format==fourcc("DXT3")||format==fourcc("DXT4")||format==fourcc("DXT5"));
            return ok?D3D_OK:0x88760866U;}; // D3DERR_NOTAVAILABLE
        b["CheckDepthStencilMatch"]=[](HleCall&)->std::uint32_t{return D3D_OK;};
        b["CheckDeviceMultiSampleType"]=[arg](HleCall& c)->std::uint32_t{return arg(c,5)==0?D3D_OK:0x88760866U;}; // no multisampling
        b["GetAdapterMonitor"]=[](HleCall&)->std::uint32_t{return 0x00010001U;};
        b["CreateDevice"]=[P,device,arg,bound,new_surface](HleCall& c)->std::uint32_t{
            std::uint32_t pp[13]={};c.guest.read(arg(c,5),pp,52); // D3DPRESENT_PARAMETERS
            P->d3d8_presentation.assign(pp,pp+13);
            const auto object=device().create(16);if(!object)return 0x8007000eU;
            // The implicit back buffer (and the depth surface when EnableAutoDepthStencil): BackBufferWidth/Height/Format, AutoDepthStencilFormat.
            const auto width=pp[0]?pp[0]:640,height=pp[1]?pp[1]:480,format=pp[2]?pp[2]:adapter_format;
            if(const auto s=new_surface(c.guest,width,height,format,1,0))bound->back_buffer=*s;
            if(pp[8])if(const auto s=new_surface(c.guest,width,height,pp[9],2,0))bound->depth=*s;
            c.guest.set_u32(arg(c,6),*object);return D3D_OK;};
        *d3d_interface=std::make_unique<HleComInterface>(*P,"IDirect3D8",methods("IDirect3D8",direct3d8,std::size(direct3d8),b));
        return **d3d_interface;};
    p.hle().add("d3d8.dll","Direct3DCreate8",{4,1,[direct3d,d3d_object](HleCall&)->std::uint32_t{
        const auto object=direct3d().create(8);*d3d_object=object.value_or(0);return *d3d_object;}});
}
} // namespace sadx
