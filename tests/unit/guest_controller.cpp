// Controller replay through the real pinned SDL DLL and Input mod. No keyboard shortcut:
// synthetic HID attachment, WM_INPUT delivery, original parser, controller mapping, Start pulses,
// repeated frame digests, and SDL axis/button reads after actual USER32 message dispatch.
#include "guest/fpu_shims.hpp"
#include "hle/hle_process.hpp"
#include "hle/hle_device_bridge.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
namespace {
using namespace sadx;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
struct PadPulse {std::uint64_t frame;std::uint32_t buttons,hold;};
struct Run {std::vector<std::uint64_t> digests;std::vector<std::uint32_t> modes;std::uint32_t filesel_799=0,filesel_last=0;std::string stop;std::vector<std::int32_t> axes;std::vector<std::uint32_t> buttons;std::uint32_t joystick_count=0;};
Run run(const std::string& dir,const std::string& system,const std::vector<PadPulse>& script,std::size_t frames,bool hotplug=false,bool overrides=false){
    std::ifstream f(dir+"/sonic.exe",std::ios::binary);require(bool(f),"cannot read "+dir+"/sonic.exe");
    std::string error;const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);require(bool(image),error);
    std::filesystem::remove_all("build/guest-controller-overlay");
    auto p=HleProcess::create(dir,"build/guest-controller-overlay",&error);require(p!=nullptr,error);
    if(!hotplug)p->host_xinput(HleXInputGamepad{});p->use_real_cpp_runtime(system);p->hle().set_logging(false);
    require(p->map_main(*image,&error),"map sonic.exe: "+error);
    if(overrides)require(install_overrides(p->guest(),process_overrides(*p,*image),&error),error);
    FpuShimStats shims;require(install_fpu_shims(p->guest(),load_fpu_sites("reference/inventory/fpu-sites.json"),shims,&error),error);
    Run r;std::size_t polygons_seen=0;
    p->on_present=[&](X86Guest& g){
        auto& d=p->d3d8;std::uint64_t h=1469598103934665603ULL;
        const auto mix=[&h](const void* at,std::size_t n){const auto* b=static_cast<const std::uint8_t*>(at);for(std::size_t i=0;i<n;++i){h^=b[i];h*=1099511628211ULL;}};
        for(const auto& [k,v]:d.render_states){mix(&k,sizeof k);mix(&v,sizeof v);}
        for(const auto& [k,v]:d.stage0_states){mix(&k,sizeof k);mix(&v,sizeof v);}
        for(;polygons_seen<d.polygons.size();++polygons_seen)mix(d.polygons[polygons_seen].vertex_bytes.data(),d.polygons[polygons_seen].vertex_bytes.size());
        const auto calls=p->hle().calls();mix(&calls,sizeof calls);mix(&p->now_100ns,sizeof p->now_100ns);
        r.digests.push_back(h);r.modes.push_back(g.u32(0x3abdc7c).value_or(0xffffffffU));
        const auto frame=r.digests.size();
        if(hotplug&&(frame==100||frame>=140))p->host_xinput(HleXInputGamepad{});
        if(hotplug&&frame==120)p->host_xinput(std::nullopt);
        if(frame==799)r.filesel_799=g.u32(0x3c5ee10).value_or(0);
        r.filesel_last=g.u32(0x3c5ee10).value_or(0); // ava_filesel_e texture 0's texaddr (0 until the texture list is loaded)
        for(const auto& k:script){if(frame==k.frame)p->host_xinput(HleXInputGamepad{static_cast<std::uint16_t>(k.buttons),0,0,0,0,0,0});if(frame==k.frame+k.hold)p->host_xinput(HleXInputGamepad{});}
        if(frame>=frames)g.stop("frames done");};
    const auto result=p->guest().call(0x643f96,{},std::nullopt,std::nullopt,0x27f,0);
    r.stop=result?std::string("returned"):result.error;
    require(r.stop=="frames done",r.stop);
    const auto* sdl=p->modules().by_name("SDL2.dll");require(sdl!=nullptr,"loaded SDL");
    const auto fn=[&](const char* name){const auto at=p->modules().export_address(sdl->base,name);require(at.has_value(),name);return *at;};
    const auto count=p->guest().call(fn("SDL_NumJoysticks"),{});require(bool(count),count.error);r.joystick_count=count.eax;
    const auto ctl=p->guest().call(fn("SDL_GameControllerOpen"),{0});require(bool(ctl)&&ctl.eax!=0,ctl.error);
    p->host_xinput(HleXInputGamepad{0xf7f4,30,180,16384,-8192,-10000,20000});
    // SDL was initialized without video. SDL_PumpEvents updates joysticks but does not drain
    // Win32 messages: use the same USER32 Peek/Dispatch boundary the game's message pump uses.
    const auto message=p->memory().allocate(28);require(message.has_value(),"message buffer");
    const auto peek=p->hle().trap(p->guest(),"USER32.dll","PeekMessageW",&error),dispatch=p->hle().trap(p->guest(),"USER32.dll","DispatchMessageW",&error);
    require(peek&&dispatch,error);
    for(unsigned q=0;q<100;++q){const auto have=p->guest().call(*peek,{*message,0,0,0,1});require(bool(have),have.error);if(!have.eax)break;const auto handled=p->guest().call(*dispatch,{*message});require(bool(handled),handled.error);}
    const auto update=p->guest().call(fn("SDL_GameControllerUpdate"),{});require(bool(update),update.error);
    // SDL correlates the raw-input device with an XInput slot after a stable match.
    // Advance the actual virtual Sleep boundary while polling, rather than changing SDL state.
    const auto sleep=p->hle().trap(p->guest(),"KERNEL32.dll","Sleep",&error);require(sleep.has_value(),error);
    for(unsigned k=0;k<10;++k){const auto waited=p->guest().call(*sleep,{10});require(bool(waited),waited.error);const auto polled=p->guest().call(fn("SDL_GameControllerUpdate"),{});require(bool(polled),polled.error);}
    for(unsigned k=0;k<6;++k){const auto v=p->guest().call(fn("SDL_GameControllerGetAxis"),{ctl.eax,k});require(bool(v),v.error);r.axes.push_back(static_cast<std::int32_t>(v.eax));}
    for(unsigned k=0;k<15;++k){const auto v=p->guest().call(fn("SDL_GameControllerGetButton"),{ctl.eax,k});require(bool(v),v.error);r.buttons.push_back(v.eax);}
    const auto close=p->guest().call(fn("SDL_GameControllerClose"),{ctl.eax});require(bool(close),close.error);
    require(p->raw_input.packets.empty(),"raw reports consumed without retained packet buffers");
    return r;
}
std::size_t first_frame_of(const Run& r,std::uint32_t mode){for(std::size_t i=0;i<r.modes.size();++i)if(r.modes[i]==mode)return i+1;return 0;}
}
int main(int argc,char** argv){try{
    require(argc==3,"usage: guest-controller-test GAME_DIR WINDOWS_SYSTEM_DIR");
    const std::vector<PadPulse> script={{450,0x10,5},{800,0x10,5}};
    const auto a=run(argv[1],argv[2],script,900);
    require(a.joystick_count==1,"one real SDL joystick");
    const auto title=first_frame_of(a,12);require(title&&title<500,"controller Start skips movie");
    require(a.filesel_799==0&&a.filesel_last!=0,"controller Start opens file select after frame 800");
    require(a.axes==std::vector<std::int32_t>({16384,8191,-10000,-20001,3854,23129}),"original SDL axis conversion");
    require(a.buttons==std::vector<std::uint32_t>({1,1,1,1,1,1,1,1,1,1,1,0,0,1,0}),"original SDL face/shoulder/stick/Guide/Start/Back/hat mapping");
    const auto b=run(argv[1],argv[2],script,900,false,true);
    require(b.digests==a.digests&&b.axes==a.axes&&b.buttons==a.buttons,"controller replay and process override equivalence");
    const auto neutral=run(argv[1],argv[2],{},500);
    require(neutral.modes.back()==8,"neutral controller does not skip movie");
    const auto plugged=run(argv[1],argv[2],{},160,true);
    require(plugged.joystick_count==1&&plugged.axes==a.axes&&plugged.buttons==a.buttons,"arrival/removal/reconnect through original SDL");
    std::cout<<"Guest controller: pinned SDL enumerates/opens 1 joystick; WM_INPUT + original HID parser/controller mapping; Start at 450 skips movie (title "<<title<<"), Start at 800 opens file select; plain/native process override 900-frame boots identical; six axes and fifteen buttons verified including Guide/triggers; neutral pad still in movie at 500; arrival/removal/reconnect verified\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
