// The pinned Speedrun limiter (DLL +10f0) waits multiplier*QPF/63 per invocation, not per Present.
// Skip the intro at 450 to cover title -> attract loading -> multiplier-2 demo -> title in 1100 frames.
// Observe the real QPC import without replacing its result. Stable frames must be paced at the live
// multiplier; loading frames may contain many waits. All clock advancement after boot is attributed
// to counter reads. The full, unskipped title path is checked separately in guest_frames.
#include "guest/fpu_shims.hpp"
#include "hle/hle_process.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
namespace {
using namespace sadx;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
std::uint64_t u64(X86Guest& g,std::uint32_t at){
    const auto lo=g.u32(at),hi=g.u32(at+4);require(lo&&hi,"unreadable timing state");
    return std::uint64_t(*lo)|(std::uint64_t(*hi)<<32);
}
}
int main(int argc,char** argv){try{
    require(argc==3,"usage: guest-timing-test GAME_DIR WINDOWS_SYSTEM_DIR");
    std::ifstream f(std::string(argv[1])+"/sonic.exe",std::ios::binary);require(bool(f),"cannot read sonic.exe");
    std::string error;const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);require(bool(image),error);
    std::filesystem::remove_all("build/guest-timing-overlay");
    auto p=HleProcess::create(argv[1],"build/guest-timing-overlay",&error);require(p!=nullptr,error);
    p->use_real_cpp_runtime(argv[2]);p->hle().set_logging(false);require(p->map_main(*image,&error),error);
    FpuShimStats shims;require(install_fpu_shims(p->guest(),load_fpu_sites("reference/inventory/fpu-sites.json"),shims,&error),error);
    std::uint32_t speedrun_base=0,wait_multiplier=0;
    std::uint64_t qpcs=0,limiter_qpcs=0,waits=0,last_time=0,frames=0,max_waits=0;
    std::size_t title_samples=0,demo_samples=0,return_samples=0;
    bool saw_demo=false,returned=false;
    auto qpc=*p->hle().find("KERNEL32.dll","QueryPerformanceCounter");const auto original=qpc.body;
    qpc.body=[&](HleCall& c){
        const auto result=original(c);++qpcs;
        try{
        if(speedrun_base&&c.guest.u32(c.guest.esp())==speedrun_base+0x115b){
            ++limiter_qpcs;
            require(c.guest.u32(speedrun_base+0x1001c)==63,"pinned Speedrun cadence changed");
            const auto multiplier=c.guest.u32(0x3b11188);require(multiplier&&(*multiplier==1||*multiplier==2),"unexpected frame multiplier");
            const auto interval=std::uint64_t(*multiplier)*u64(c.guest,0x3d08538)/63;
            const auto elapsed=p->now_100ns-u64(c.guest,0x3d08550);
            if(elapsed>=interval){require(elapsed-interval<p->counter_step_100ns,"limiter overshot its interval");++waits;wait_multiplier=*multiplier;}
        }
        }catch(const std::exception& e){c.guest.stop(e.what());}
        return result;
    };
    p->hle().add("KERNEL32.dll","QueryPerformanceCounter",qpc);
    p->on_present=[&](X86Guest& g){try{
        ++frames;
        if(frames==1){
            const auto* mod=p->modules().by_name("sadx-speedrun-mod.dll");require(mod!=nullptr,"Speedrun mod was not loaded");speedrun_base=mod->base;
        }else{
            require(p->now_100ns-last_time==qpcs*10,"virtual time is not fully explained by 1 us QPC reads");
        }
        if(frames>=500){
            const auto mode=g.u32(0x3abdc7c),multiplier=g.u32(0x3b11188);
            require(mode&&multiplier,"missing mode/multiplier");
            const auto delta=p->now_100ns-last_time;
            if(*mode==5)saw_demo=true;
            if(saw_demo&&*mode==12)returned=true;
            if(waits==1){
                const auto expected=(std::uint64_t(wait_multiplier)*10'000'000/63+9)/10*10;
                require(delta==expected,"Present with one limiter wait has wrong cadence at frame "+std::to_string(frames)+": "+std::to_string(delta));
                require(limiter_qpcs+1==qpcs,"steady frame has unexpected QPC callers");
                if(*mode==12&&wait_multiplier==1){if(returned)++return_samples;else ++title_samples;}
                if(*mode==5&&wait_multiplier==2)++demo_samples;
            }
            max_waits=std::max(max_waits,waits);
        }
        if(frames==450)p->host_key(13,true);if(frames==455)p->host_key(13,false);
        last_time=p->now_100ns;qpcs=limiter_qpcs=waits=0;
        if(frames==1100)g.stop("frames done");
        }catch(const std::exception& e){g.stop(e.what());}
    };
    const auto result=p->guest().call(0x643f96,{},std::nullopt,std::nullopt,0x27f,0);
    require(result.error=="frames done",result.error);
    require(title_samples>=400&&demo_samples>=60&&return_samples>=50,"title/demo/return timing coverage missing");
    require(max_waits>1,"attract loading did not exercise multiple limiter waits per Present");
    std::cout<<"Guest timing: "<<frames<<" frames; "<<title_samples<<" title and "<<return_samples<<" returned-title samples at 15.873 ms, "<<demo_samples
             <<" multiplier-2 attract samples at 31.746 ms; up to "<<max_waits<<" limiter waits per loading Present; every wait matches multiplier*10 MHz/63 within one QPC step, all virtual time after boot explained by 1 us QPC reads (assumed clock, not Windows wall time)\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
