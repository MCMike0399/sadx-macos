#include "hle/hle_task_bridge.hpp"
#include "hle/hle_process.hpp"
#include "engine/task_work.hpp"
#include <array>
#include <algorithm>
#include <unicorn/unicorn.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
namespace {
using namespace sadx;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
struct Fixture {
    std::unique_ptr<HleProcess> p;bool mutant=false;
    Fixture(const ExecutableImage& image,bool native){std::string error;p=HleProcess::create(".","build/task-bridge-overlay",&error);require(p!=nullptr,error);require(p->map_main(image,&error),error);
        if(native)for(auto entry:task_overrides(*p)){const auto bridge=entry.bridge;const auto at=entry.address;require(p->guest().add_override(at,[this,bridge,at](X86Guest& g){if(mutant&&at!=0x40b1a0)g.set_u32(g.esp()+4,g.u32(g.esp()+4).value_or(0)^1U);return bridge(g);},&error),error);}}
};
void compare(Fixture& a,Fixture& b,std::uint32_t entry,const std::vector<std::uint32_t>& args){const auto x=a.p->guest().call(entry,args),y=b.p->guest().call(entry,args);require(bool(x),"original: "+x.error);require(bool(y),"native: "+y.error);
    if(entry!=0x40b310)require(x.eax==y.eax,"task-work EAX");
    for(const auto at:{0x3abdbe8U,0x3abdbf0U,0x3c839fcU,0x3c83a00U})require(a.p->guest().u32(at)==b.p->guest().u32(at),"task-work globals");
    const auto& l=a.p->hle().log();const auto& r=b.p->hle().log();require(l.size()==r.size(),"task-work trace count");
    for(unsigned k=0;k<l.size();++k)require(l[k].dll==r[k].dll&&l[k].symbol==r[k].symbol&&l[k].return_address==r[k].return_address&&l[k].arguments==r[k].arguments&&l[k].result==r[k].result,"task-work exact HLE trace");
}
void memory(Fixture& a,Fixture& b){const auto base=a.p->guest().u32(0x3abdbf0).value_or(0);std::vector<std::uint8_t> x(TaskWorkArena::usable_bytes+32),y(x.size());require(a.p->guest().read(base,x.data(),x.size())&&b.p->guest().read(base,y.data(),y.size()),"arena memory");require(x==y,"task-work full arena bytes");}
}
int main(int argc,char** argv){try{
    require(argc==3,"usage: hle-task-bridge-test SONIC.EXE CORPUS");std::string error;std::ifstream f(argv[1],std::ios::binary);const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);require(image.has_value(),error);
    std::ifstream input(argv[2],std::ios::binary);const std::string bytes((std::istreambuf_iterator<char>(input)),{});require(!bytes.empty()&&bytes.size()%4==0,"task corpus");std::vector<std::uint32_t> words(bytes.size()/4);std::memcpy(words.data(),bytes.data(),bytes.size());
    Fixture original(*image,false),native(*image,true);unsigned sequences=0,operations=0;
    for(std::size_t offset=0;offset<words.size();){const auto count=words[offset++];require(count<=(words.size()-offset)/2,"sequence bounds");
        for(auto* p:{&original,&native}){p->p->hle().clear_log();const auto base=p->p->guest().u32(0x3abdbf0).value_or(0);if(base)p->p->guest().set_u32(base+TaskWorkArena::usable_bytes+16,0x12345678U+sequences);}
        compare(original,native,0x40b1a0,{});memory(original,native);std::vector<std::uint32_t> allocations;
        for(unsigned step=0;step<count;++step){const auto op=words[offset++],value=words[offset++];require(op<2,"operation");require(!op||value<allocations.size(),"allocation index");const auto argument=op?allocations[value]:value;
            compare(original,native,op?0x40b310:0x40b220,{argument});
            if(!op){const auto addr=original.p->guest().reg(UC_X86_REG_EAX);allocations.push_back(addr);
                // Game code owns payload writes. Allocation/free must preserve them,
                // including bytes subsequently exposed by first-fit reuse/coalescing.
                if(addr){const auto next=original.p->guest().u32(addr-24);require(next&&*next>=addr,"physical next");const auto capacity=*next-addr;if(capacity){const auto n=std::min<std::uint32_t>(capacity,16);std::array<std::uint8_t,16> pattern{};for(unsigned k=0;k<n;++k)pattern[k]=static_cast<std::uint8_t>(step+k+sequences);for(auto* p:{&original,&native}){p->p->guest().write(addr,pattern.data(),n);p->p->guest().write(addr+capacity-n,pattern.data(),n);}}}
            }++operations;
        }memory(original,native);++sequences;
    }
    unsigned caught=0;
    for(const auto entry:{0x40b220U,0x40b310U}){
        compare(original,native,0x40b1a0,{});compare(original,native,0x40b220,{64});const auto address=original.p->guest().reg(UC_X86_REG_EAX);native.mutant=true;
        try{compare(original,native,entry,{entry==0x40b220?64U:address});}catch(const std::exception&){++caught;}native.mutant=false;
    }require(caught==2,"task-work bridge mutants");
    auto& g=native.p->guest();require(bool(g.call(0x40b1a0,{})),"native reset");const auto base=g.u32(0x3abdbf0).value_or(0);g.set_u32(base+8,base+64);
    const auto corrupt=g.call(0x40b220,{32});require(!corrupt&&corrupt.error.find("changed metadata")!=std::string::npos,"external metadata corruption boundary");
    require(bool(g.call(0x40b1a0,{})),"native reset after corruption");const auto wrapped=g.call(0x40b220,{0xffffffffU});require(!wrapped&&wrapped.error.find("40b220")!=std::string::npos,"declared size-wrap boundary");
    const auto block=g.call(0x40b220,{32});require(bool(block)&&block.eax!=0,"native allocation");require(bool(g.call(0x40b310,{block.eax})),"native release");const auto twice=g.call(0x40b310,{block.eax});require(!twice&&twice.error.find("40b31c")!=std::string::npos,"double/foreign free boundary");
    std::cout<<"Task bridge: "<<sequences<<" complete oracle sequences, "<<operations<<" operations; actual CRT/heap acquisition and repeated initialization; EAX allocation/global/exact HLE traces and full 16 MiB+32 arena bytes match with guest payload writes and sentinel padding; 2 input mutants caught; corrupt metadata, wrap and unowned free stop explicitly\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
