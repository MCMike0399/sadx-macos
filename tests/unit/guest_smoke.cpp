#include "guest/x86_guest.hpp"
#include "game/game_frame_control.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
// The first override-equivalence check (ROADMAP M1): sonic.exe runs inside the process on
// the software CPU; 7899a0 (the frame multiplier: 389d7dc, 3d084a8/ac from 3d08538/3c) is
// executed from its original bytes and compared with the native port, then replaced by an
// override and the guest's caller sees the override's results instead.
//   guest-smoke-test SONIC.EXE
namespace {using namespace sadx;void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(int argc,char** argv){try{
    require(argc==2,"usage: guest-smoke-test SONIC.EXE");
    std::ifstream f(argv[1],std::ios::binary);require(bool(f),"cannot read sonic.exe");
    std::string error;auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);require(bool(image),error.c_str());
    auto guest=X86Guest::create(&error);require(guest!=nullptr,error.c_str());
    require(guest->map_image(*image,&error),error.c_str());
    const std::uint32_t cases[][3]={{10000000u,0u,1u},{3579545u,0u,2u},{0x12345678u,0x9u,1u},{0u,0u,3u}};
    for(const auto& c:cases){
        guest->set_u32(0x3d08538,c[0]);guest->set_u32(0x3d0853c,c[1]);guest->set_u32(0x389d7dc,0);guest->set_u32(0x3d084a8,0);guest->set_u32(0x3d084ac,0);
        const auto r=guest->call(0x7899a0,{c[2]});require(bool(r),r.error.c_str());
        GameFrameRendererState native{};native.frequency_low_at_3d08538=c[0];native.frequency_high_at_3d0853c=c[1];native.frame_multiplier_at_389d7dc=0;native.interval_low_at_3d084a8=0;native.interval_high_at_3d084ac=0;
        require(bool(set_game_frame_multiplier(static_cast<std::int32_t>(c[2]),native)),"native 7899a0");
        require(guest->u32(0x389d7dc)==static_cast<std::uint32_t>(*native.frame_multiplier_at_389d7dc),"389d7dc: guest == native");
        require(guest->u32(0x3d084a8)==*native.interval_low_at_3d084a8&&guest->u32(0x3d084ac)==*native.interval_high_at_3d084ac,"3d084a8/ac: guest == native");
    }
    // An override replaces the guest function: the caller sees what the native owner wrote.
    bool ran=false;
    require(guest->add_override(0x7899a0,[&](X86Guest& g){ran=true;g.set_u32(0x389d7dc,0x77);return 0U;},&error),error.c_str()); // cdecl: caller pops
    guest->set_u32(0x389d7dc,0);const auto r=guest->call(0x7899a0,{1u});
    require(r&&ran&&guest->u32(0x389d7dc)==0x77u,"override ran instead of the original and returned to the stop address");
    std::cout<<"Guest smoke: 7899a0 from original bytes equals the native port on 4 cases; override replaces it.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
