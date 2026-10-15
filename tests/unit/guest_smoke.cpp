#include "guest/x86_guest.hpp"
#include "engine/game_frame_control.hpp"
#include <cstring>
#include <unicorn/unicorn.h>
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
    // Tail transfer resumes a guest suffix on the actual caller frame. Its RET 4
    // consumes the original argument, ignoring the replaced bridge's pop count.
    constexpr std::uint32_t code=0x10000000,suffix=code+0x100;
    require(guest->map(code,0x1000,true,&error),error.c_str());
    const std::uint8_t ret4[]={0x8b,0x44,0x24,0x04,0xc2,0x04,0x00}; // MOV EAX,[ESP+4]; RET 4
    require(guest->write(suffix,ret4,sizeof ret4),"write tail fixture");
    std::uint32_t entry_esp=0,entry_back=0;bool suffix_seen=false;
    require(guest->add_instruction_hook(suffix,[&](X86Guest& g,std::uint32_t){
        suffix_seen=true;require(g.esp()==entry_esp&&g.u32(g.esp())==entry_back,"tail preserves ESP/return word");
        require(g.u32(g.esp()+4)==0x12345678U,"tail preserves caller argument");
        require(g.reg(UC_X86_REG_ECX)==0x2468U&&g.reg(UC_X86_REG_EDX)==0x369cU,"tail preserves registers");
        return false;},&error),error.c_str());
    require(guest->add_override(code,[&](X86Guest& g){
        entry_esp=g.esp();entry_back=*g.u32(g.esp());g.tail_redirect(suffix);return 20U;
    },&error),error.c_str());
    const auto tail=guest->call(code,{0x12345678U},0x2468U,0x369cU);
    require(tail&&suffix_seen&&tail.eax==0x12345678U&&tail.esp==entry_esp+8,"tail executes suffix RET with the original ABI");
    // A continuation can tail-transfer too, after redirect has restored its caller.
    require(guest->add_override(code+0x200,[&](X86Guest& g){
        entry_esp=g.esp();entry_back=*g.u32(g.esp());
        g.redirect(code+0x300,{},0,[&](X86Guest& resumed){resumed.tail_redirect(suffix);});return 0U;
    },&error),error.c_str());
    const std::uint8_t ret=0xc3;require(guest->write(code+0x300,&ret,1),"write redirect fixture");
    suffix_seen=false;const auto chained=guest->call(code+0x200,{0x12345678U},0x2468U,0x369cU);
    require(chained&&suffix_seen&&chained.eax==0x12345678U&&chained.esp==entry_esp+8,"continuation tail preserves the owner caller frame");
    std::cout<<"Guest tail transfer: original stack/return word/arguments/registers preserved, suffix RET 4 and continuation chain passed.\n";
    std::cout<<"Guest smoke: 7899a0 from original bytes equals the native port on 4 cases; override replaces it.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
