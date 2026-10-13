// Exercise both D3D8 GetDeviceCaps COM interfaces as guest calls (no game boot needed).
#include "engine/d3d8_caps.hpp"
#include "hle/hle_process.hpp"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>
namespace {
using namespace sadx;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
std::uint32_t method(X86Guest& g,std::uint32_t object,unsigned slot){
    const auto table=g.u32(object);require(table.has_value(),"COM vtable");
    const auto target=g.u32(*table+4*slot);require(target.has_value(),"COM method");return *target;
}
}
int main(int argc,char** argv){try{
    require(argc==2,"usage: guest-d3d8-caps-test SONIC.EXE");
    std::string error;auto p=HleProcess::create(".","build/guest-caps-overlay",&error);require(p!=nullptr,error);
    auto& g=p->guest();
    const auto create=p->hle().trap(g,"d3d8.dll","Direct3DCreate8",&error);require(create.has_value(),error);
    const auto d3d=g.call(*create,{0xdc});require(bool(d3d)&&d3d.eax!=0,d3d.error);
    const auto buffer=p->memory().allocate(0xd4+8),parameters=p->memory().allocate(52),device_out=p->memory().allocate(4);
    require(buffer&&parameters&&device_out,"guest memory");
    const auto expected=assumed_d3d8_caps();
    const auto check=[&](){
        std::array<std::uint8_t,0xd4+8> got{};require(g.read(*buffer,got.data(),got.size()),"read caps output");
        require(std::equal(expected.begin(),expected.end(),got.begin()+4),"GetDeviceCaps wire bytes");
        for(unsigned k=0;k<4;++k)require(got[k]==0xa5&&got[k+0xd8]==0xa5,"GetDeviceCaps output bounds");
    };
    const auto poison=[&](){std::array<std::uint8_t,0xd4+8> bytes;bytes.fill(0xa5);require(g.write(*buffer,bytes.data(),bytes.size()),"poison caps output");};
    poison();const auto adapter_caps=g.call(method(g,d3d.eax,13),{d3d.eax,0,1,*buffer+4});
    require(bool(adapter_caps)&&adapter_caps.eax==0,adapter_caps.error);check();
    const std::uint32_t pp[13]={640,480,22,1,0,1,0,1,0,0,0,0,0};
    require(g.write(*parameters,pp,sizeof pp),"presentation parameters");
    const auto device=g.call(method(g,d3d.eax,15),{d3d.eax,0,1,0,0x20,*parameters,*device_out});
    require(bool(device)&&device.eax==0,device.error);
    const auto object=g.u32(*device_out);require(object&&*object,"created device");
    poison();const auto device_caps=g.call(method(g,*object,7),{*object,*buffer+4});
    require(bool(device_caps)&&device_caps.eax==0,device_caps.error);check();
    require(std::count(p->assumptions.begin(),p->assumptions.end(),d3d8_caps_assumption())==1,"shared caps assumption reported once");
    // Original 78cfa0 has no callees. Its MIPMAP test clears the sixteen mipmap-format flags
    // at 389d47c + 8*k when bit 0x4000 is absent. Check the real consumer, including the negative case.
    std::ifstream f(argv[1],std::ios::binary);require(bool(f),"cannot read sonic.exe");
    const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);require(image.has_value(),error);
    require(g.map_image(*image,&error),error);
    std::array<std::uint32_t,16> initial_flags{};
    for(unsigned k=0;k<16;++k){const auto v=g.u32(0x389d47c+8*k);require(v.has_value(),"mipmap flags");initial_flags[k]=*v;}
    require(g.write(0x3d127c0,expected.data(),expected.size()),"copy caps to source owner");
    const auto enabled=g.call(0x78cfa0,{});require(bool(enabled),enabled.error);
    for(unsigned k=0;k<16;++k)require(g.u32(0x389d47c+8*k)==initial_flags[k],"MIPMAP capability preserves original format flags");
    require(initial_flags[1]==1,"twiddled-mipmap (0x200) format initially has its mipmap flag");
    require(g.set_u32(0x3d127fc,0x1ac05),"remove MIPMAP capability only");
    const auto disabled=g.call(0x78cfa0,{});require(bool(disabled),disabled.error);
    for(unsigned k=0;k<16;++k)require(g.u32(0x389d47c+8*k)==0,"missing MIPMAP capability clears original format flags");
    std::cout<<"Guest D3D8 caps: adapter and device COM calls return identical 0xd4-byte HAL/mipmap/cube-map/WFOG caps, output bounds preserved, reference-GPU assumption reported once; original 78cfa0 preserves mipmap flags with MIPMAP and clears them without it\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
