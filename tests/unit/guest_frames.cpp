// M1 4f: frames, determinism and FPS in the guest. sonic.exe boots from its entry point with the FPU shims and
// runs 300 frames (to the 300th IDirect3DDevice8::Present; the intro FMV's DirectShow graph comes later). Each
// Present closes a frame digest (FNV-1a over the recorded D3D8 state, the frame's DrawPrimitiveUP vertex bytes,
// the HLE call count and the virtual clock); a second boot must produce the same 300 digests. Reported: host
// frames per second (the guest's emulation speed, boot excluded) and the virtual frame time, which comes from
// the Speedrun mod's frame limiter running on the virtual QueryPerformanceCounter.
//   guest-frames-test GAME_DIR WINDOWS_SYSTEM_DIR [frames]
#include "guest/fpu_shims.hpp"
#include "hle/hle_process.hpp"
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
namespace {
using namespace sadx;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
struct Hash {std::uint64_t h=1469598103934665603ULL;
    void bytes(const void* p,std::size_t n){const auto* b=static_cast<const std::uint8_t*>(p);for(std::size_t i=0;i<n;++i){h^=b[i];h*=1099511628211ULL;}}
    template<class T>void value(const T& v){bytes(&v,sizeof v);}};
struct Run {std::vector<std::uint64_t> digests;std::vector<std::uint64_t> frame_time_100ns;double seconds=0;std::uint64_t hle_calls=0;std::string stop;FpuShimStats shims;};
Run run(const std::string& dir,const std::string& system,std::size_t frames){
    std::ifstream f(dir+"/sonic.exe",std::ios::binary);require(bool(f),"cannot read "+dir+"/sonic.exe");
    std::string error;const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);require(bool(image),error);
    std::filesystem::remove_all("build/guest-frames-overlay");
    auto p=HleProcess::create(dir,"build/guest-frames-overlay",&error);require(p!=nullptr,error);
    p->use_real_cpp_runtime(system);p->hle().set_logging(false);
    require(p->map_main(*image,&error),"map sonic.exe: "+error);
    Run r;require(install_fpu_shims(p->guest(),load_fpu_sites("reference/inventory/fpu-sites.json"),r.shims,&error),error);
    std::size_t polygons_seen=0;std::uint64_t last_time=0;std::chrono::steady_clock::time_point first_present;
    p->on_present=[&](X86Guest& g){
        auto& d=p->d3d8;Hash h;
        for(const auto& [k,v]:d.render_states){h.value(k);h.value(v);}
        for(const auto& [k,v]:d.stage0_states){h.value(k);h.value(v);}
        for(const auto& [k,m]:d.transforms){h.value(k);h.bytes(m.data(),64);}
        for(;polygons_seen<d.polygons.size();++polygons_seen){const auto& poly=d.polygons[polygons_seen];h.value(poly.primitive_type);h.value(poly.primitive_count);h.bytes(poly.vertex_bytes.data(),poly.vertex_bytes.size());}
        h.value(d.begin_scenes);h.value(d.end_scenes);h.value(d.clears);h.value(d.render_calls);h.value(d.texture_binds);
        h.value(p->hle().calls());h.value(p->now_100ns);
        r.digests.push_back(h.h);
        if(r.digests.size()==1)first_present=std::chrono::steady_clock::now();else r.frame_time_100ns.push_back(p->now_100ns-last_time);
        last_time=p->now_100ns;
        if(r.digests.size()>=frames)g.stop("frames done");};
    const auto result=p->guest().call(0x643f96,{});
    r.seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-first_present).count();
    r.stop=result?std::string("returned"):result.error;r.hle_calls=p->hle().calls();
    return r;
}
}
int main(int argc,char** argv){try{
    require(argc==3||argc==4,"usage: guest-frames-test GAME_DIR WINDOWS_SYSTEM_DIR [frames]");
    const std::size_t frames=argc==4?std::stoul(argv[3]):300;
    const auto a=run(argv[1],argv[2],frames);
    require(a.stop=="frames done"&&a.digests.size()==frames,"the guest runs "+std::to_string(frames)+" frames; it stopped after "+std::to_string(a.digests.size())+": "+a.stop);
    const auto b=run(argv[1],argv[2],frames);
    require(b.digests==a.digests,"a second boot produces different frame digests");
    // Virtual frame time over the frames after the first 60 (start-up frames load data).
    double sum=0;std::size_t n=0;for(std::size_t i=60;i<a.frame_time_100ns.size();++i){sum+=static_cast<double>(a.frame_time_100ns[i]);++n;}
    const double frame_ms=n?sum/n/10000.0:0;
    std::ostringstream fps;fps<<std::fixed<<std::setprecision(1)<<(frames-1)/a.seconds;
    std::cout<<"Guest frames: "<<frames<<" frames, identical digests on two boots; guest FPS "<<fps.str()<<" (host, boot excluded); virtual frame time "
             <<std::setprecision(3)<<frame_ms<<" ms ("<<std::setprecision(2)<<(frame_ms>0?1000/frame_ms:0)<<" fps under the Speedrun limiter); "<<a.hle_calls<<" HLE calls\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
