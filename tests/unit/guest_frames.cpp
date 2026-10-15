// M1 4f: frames, determinism and FPS in the guest. sonic.exe boots from its entry point with the FPU shims and
// runs from the SEGA logo through the intro movie (the headless DirectShow graph, src/hle/hle_dshow) to the title
// screen: GameMode (3abdc7c) must go Logo (1) -> Movie (8) -> Menu (12) and the run ends `extra` frames after the
// title is first reached. Each IDirect3DDevice8::Present closes a frame digest (FNV-1a over the recorded D3D8
// state, the frame's DrawPrimitiveUP vertex bytes, the HLE call count and the virtual clock); every frame's D3D8
// stream must be non-empty (a Clear or a draw), and a second boot must produce the same digests; a third boot with
// the override table installed (src/guest/override_table) must produce the same first 600 digests. Reported: host
// frames per second (the guest's emulation speed, boot excluded), the frames of each mode change, and the virtual
// frame time on the SEGA logo, which comes from the Speedrun mod's frame limiter on the virtual QueryPerformanceCounter.
//   guest-frames-test GAME_DIR WINDOWS_SYSTEM_DIR [extra frames after the title, default 60]
#include "guest/fpu_shims.hpp"
#include "guest/override_table.hpp"
#include "hle/hle_process.hpp"
#include "hle/hle_device_bridge.hpp"
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
namespace {
using namespace sadx;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
struct Hash {std::uint64_t h=1469598103934665603ULL;
    void bytes(const void* p,std::size_t n){const auto* b=static_cast<const std::uint8_t*>(p);for(std::size_t i=0;i<n;++i){h^=b[i];h*=1099511628211ULL;}}
    template<class T>void value(const T& v){bytes(&v,sizeof v);}};
struct Run {std::vector<std::pair<std::size_t,std::uint32_t>> modes;std::size_t empty_frames=0;std::vector<std::uint64_t> digests;std::vector<std::uint64_t> frame_time_100ns;double seconds=0;std::uint64_t hle_calls=0;std::string stop;FpuShimStats shims;std::map<std::string,std::size_t> override_calls;};
Run run(const std::string& dir,const std::string& system,std::size_t extra,std::size_t limit=0){
    std::ifstream f(dir+"/sonic.exe",std::ios::binary);require(bool(f),"cannot read "+dir+"/sonic.exe");
    std::string error;const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);require(bool(image),error);
    std::filesystem::remove_all("build/guest-frames-overlay");
    auto p=HleProcess::create(dir,"build/guest-frames-overlay",&error);require(p!=nullptr,error);
    p->use_real_cpp_runtime(system);p->hle().set_logging(false);
    require(p->map_main(*image,&error),"map sonic.exe: "+error);
    Run r;require(install_fpu_shims(p->guest(),load_fpu_sites("reference/inventory/fpu-sites.json"),r.shims,&error),error);
    if(limit)for(const auto& entry:process_overrides(*p,*image)){
        r.override_calls.emplace(entry.name,0);
        const auto bridge=entry.bridge;const auto name=entry.name;
        require(p->guest().add_override(entry.address,[bridge,name,&r](X86Guest& g){
            ++r.override_calls.at(name);return bridge(g);},&error),error);
    }
    std::size_t polygons_seen=0,title_frame=0,draws_seen=0,clears_seen=0;std::uint64_t last_time=0;std::chrono::steady_clock::time_point first_present;
    p->on_present=[&](X86Guest& g){
        auto& d=p->d3d8;Hash h;
        for(const auto& [k,v]:d.render_states){h.value(k);h.value(v);}
        for(const auto& [k,v]:d.stage0_states){h.value(k);h.value(v);}
        for(const auto& [k,m]:d.transforms){h.value(k);h.bytes(m.data(),64);}
        for(;polygons_seen<d.polygons.size();++polygons_seen){const auto& poly=d.polygons[polygons_seen];h.value(poly.primitive_type);h.value(poly.primitive_count);h.bytes(poly.vertex_bytes.data(),poly.vertex_bytes.size());}
        h.value(d.begin_scenes);h.value(d.end_scenes);h.value(d.clears);h.value(d.render_calls);h.value(d.texture_binds);
        h.value(p->hle().calls());h.value(p->now_100ns);
        r.digests.push_back(h.h);
        if(d.render_calls==draws_seen&&d.clears==clears_seen)++r.empty_frames;draws_seen=d.render_calls;clears_seen=d.clears;
        const auto mode=g.u32(0x3abdc7c).value_or(0xffffffffU);if(r.modes.empty()||r.modes.back().second!=mode)r.modes.push_back({r.digests.size(),mode});
        if(mode==12&&!title_frame)title_frame=r.digests.size();
        if(std::getenv("GUEST_FRAMES_PROGRESS")&&r.digests.size()%500==0)std::cerr<<"  frame "<<r.digests.size()<<" mode "<<mode<<" t "<<p->now_100ns/10000<<" ms\n";
        if(r.digests.size()==1)first_present=std::chrono::steady_clock::now();else r.frame_time_100ns.push_back(p->now_100ns-last_time);
        last_time=p->now_100ns;
        if((title_frame&&r.digests.size()>=title_frame+extra)||(limit&&r.digests.size()>=limit))g.stop("frames done");
        if(r.digests.size()>=12000)g.stop("no title screen in 12000 frames");};
    const auto result=p->guest().call(0x643f96,{},std::nullopt,std::nullopt,0x27f,0); // no wall-clock limit: the run ends on frames, not host speed
    r.seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-first_present).count();
    r.stop=result?std::string("returned"):result.error;r.hle_calls=p->hle().calls();
    return r;
}
}
int main(int argc,char** argv){try{
    require(argc==3||argc==4,"usage: guest-frames-test GAME_DIR WINDOWS_SYSTEM_DIR [extra frames]");
    const std::size_t extra=argc==4?std::stoul(argv[3]):60;
    const auto a=run(argv[1],argv[2],extra);
    std::ostringstream modes;for(const auto& [frame,mode]:a.modes)modes<<" "<<mode<<"@"<<frame;
    require(a.stop=="frames done","the guest reaches the title screen; it stopped after "+std::to_string(a.digests.size())+" frames: "+a.stop+" (modes"+modes.str()+")");
    // Logo (1), then Movie (8), then Menu (12), in that order (other modes pass in between: 24 on the first frame, 11 for one frame).
    std::size_t logo=0,movie=0,title=0;
    for(const auto& [frame,mode]:a.modes){if(mode==1&&!logo)logo=frame;else if(mode==8&&logo&&!movie)movie=frame;else if(mode==12&&movie&&!title)title=frame;}
    require(logo&&movie&&title,"GameMode goes Logo -> Movie -> Menu:"+modes.str());
    require(a.empty_frames==0,std::to_string(a.empty_frames)+" frames with an empty D3D8 stream");
    // Keep the full movie path's steady title cadence separate from transition/loading frames.
    const auto title_samples=std::min<std::size_t>(extra,60);
    require(title_samples>0,"at least one post-title timing sample is required");
    for(std::size_t frame=title+1;frame<=title+title_samples;++frame)
        require(a.frame_time_100ns.at(frame-2)==158730,"full-intro title cadence is not 63 FPS at frame "+std::to_string(frame));
    const auto b=run(argv[1],argv[2],extra);
    require(b.stop=="frames done","the second boot stopped after "+std::to_string(b.digests.size())+" frames: "+b.stop);
    require(b.digests==a.digests,"a second boot produces different frame digests");
    // A third boot with every override of the table installed: the first 600 frames must hash the same.
    const auto o=run(argv[1],argv[2],extra,600);
    require(o.stop=="frames done","the boot with the override table stopped after "+std::to_string(o.digests.size())+" frames: "+o.stop);
    require(o.digests.size()==600&&std::equal(o.digests.begin(),o.digests.end(),a.digests.begin()),"with the override table installed the first 600 frame digests differ");
    // Virtual frame time on the SEGA logo after the first 60 frames (start-up frames load data).
    double sum=0;std::size_t n=0;for(std::size_t i=60;i+1<movie&&i<a.frame_time_100ns.size();++i){sum+=static_cast<double>(a.frame_time_100ns[i]);++n;}
    const double frame_ms=n?sum/n/10000.0:0;
    std::ostringstream fps;fps<<std::fixed<<std::setprecision(1)<<static_cast<double>(a.digests.size()-1)/a.seconds;
    std::cout<<"Guest frames: "<<a.digests.size()<<" frames, SEGA logo -> intro movie -> title (logo at frame "<<logo<<", movie at "<<movie<<", title at "<<title<<"; GameMode"<<modes.str()<<"), every frame non-empty, identical digests on two boots and on the first 600 with the override table installed; guest FPS "<<fps.str()
             <<" (host, boot excluded); virtual frame time on the logo "<<std::setprecision(3)<<frame_ms<<" ms ("<<std::setprecision(2)<<(frame_ms>0?1000/frame_ms:0)<<" fps under the Speedrun limiter); "<<a.hle_calls<<" HLE calls; "<<title_samples<<" full-intro title frames at 15.873 ms (63 FPS)\n";
    std::cout<<"  override calls through frame 600 (including boot):";
    for(const auto& [name,count]:o.override_calls)std::cout<<" "<<name<<" x"<<count;
    std::cout<<'\n';
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
