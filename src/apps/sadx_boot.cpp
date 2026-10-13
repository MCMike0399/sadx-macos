// sadx-boot: run the native composition of sonic.exe's game loop with the
// updated speedrun profile and report how far native execution gets.
//   sadx-boot SONIC.EXE INPUT_MOD.DLL [frames] [--survey]
#include "runtime/game_session.hpp"
#include "platform/mac/mac_clock.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
int main(int argc,char** argv){
    if(argc<3){std::fprintf(stderr,"usage: %s SONIC.EXE INPUT_MOD.DLL [frames] [--survey]\n",argv[0]);return 2;}
    const auto frames=argc>3&&argv[3][0]!='-'?static_cast<std::uint32_t>(std::strtoul(argv[3],nullptr,10)):1U;
    auto policy=sadx::OwnerPolicy::stop_at_unported;
    for(int i=3;i<argc;++i)if(!std::strcmp(argv[i],"--survey"))policy=sadx::OwnerPolicy::survey_unported_as_noop;
    sadx::MacPerformanceCounter counter;
    auto loaded=sadx::load_boot_config(argv[1],argv[2],frames,policy,counter.limiter_clock());
    if(!loaded){std::fprintf(stderr,"%s\n",loaded.error.c_str());return 2;}
    sadx::NativeGame game(*loaded.config);const auto r=game.run();
    std::fputs(sadx::describe_report(r,game.d3d8(),policy).c_str(),stdout);
    return r.failure.empty()?0:1;
}
