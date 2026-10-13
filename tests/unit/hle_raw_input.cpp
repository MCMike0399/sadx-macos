#include "hle/hle_process.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
namespace {using namespace sadx;void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(){try{
    std::string error;auto p=HleProcess::create(".","build/raw-input-overlay",&error);require(p!=nullptr,"process");
    auto& g=p->guest();const auto memory=p->memory().allocate(2048);require(memory.has_value(),"memory");
    const auto base=*memory,size=base+512,descriptor=base+520,registration=base+528,report=base+640,list=base+768;
    const auto call=[&](const char* dll,const char* name,std::initializer_list<std::uint32_t> args){const auto at=p->hle().trap(g,dll,name,&error);require(at.has_value(),"trap");return g.call(*at,args);};
    g.set_u32(size,99);auto r=call("USER32.dll","GetRawInputDeviceList",{0,size,8});require(bool(r)&&r.eax==0&&g.u32(size)==0,"no pad");
    p->host_xinput(HleXInputGamepad{});
    r=call("USER32.dll","GetRawInputDeviceList",{0,size,8});require(bool(r)&&g.u32(size)==1,"count");
    g.set_u32(size,0);r=call("USER32.dll","GetRawInputDeviceList",{base,size,8});require(bool(r)&&r.eax==0xffffffff&&p->last_error==122&&g.u32(size)==1,"small device list");
    r=call("USER32.dll","GetRawInputDeviceList",{base,size,8});require(bool(r)&&r.eax==1&&g.u32(base)==HleRawInputState::device&&g.u32(base+4)==2,"raw HID entry");
    g.set_u32(size,32);r=call("USER32.dll","GetRawInputDeviceInfoA",{HleRawInputState::device,0x2000000b,base,size});
    require(bool(r)&&r.eax==32&&g.u32(base+8)==0x45e&&g.u32(base+12)==0x28e&&g.u32(base+20)==0x50001,"device info layout");
    g.set_u32(size,0);r=call("USER32.dll","GetRawInputDeviceInfoA",{HleRawInputState::device,0x20000005,0,size});require(bool(r)&&g.u32(size)==8,"opaque descriptor size");
    r=call("USER32.dll","GetRawInputDeviceInfoA",{HleRawInputState::device,0x20000005,descriptor,size});require(bool(r)&&r.eax==8,"descriptor copy");
    r=call("hid.dll","HidP_GetCaps",{descriptor,base});require(bool(r)&&r.eax==0x110000&&g.u32(base)==0x10005&&g.u32(base+48)==0x100006,"caps counts");
    HleWindow window;window.handle=0x10010;p->windows.windows[window.handle]=window;
    const std::uint32_t reg[3]={0x50001,0x2100,0x10010};g.write(registration,reg,sizeof reg);
    r=call("USER32.dll","RegisterRawInputDevices",{registration,1,12});require(bool(r)&&r.eax==1,"registration");
    require(p->windows.queue.size()==1&&p->windows.queue.back().message==0xff,"initial report");
    const auto token=p->windows.queue.back().lparam;
    g.set_u32(size,0);r=call("USER32.dll","GetRawInputData",{token,0x10000003,0,size,16});require(bool(r)&&g.u32(size)==41,"packet size");
    g.set_u32(size,40);r=call("USER32.dll","GetRawInputData",{token,0x10000003,report,size,16});require(bool(r)&&r.eax==0xffffffff&&p->last_error==122,"small packet buffer");
    g.set_u32(size,41);r=call("USER32.dll","GetRawInputData",{token,0x10000003,report,size,16});require(bool(r)&&r.eax==41&&g.u32(report+16)==17,"report wire layout");
    r=call("USER32.dll","GetRawInputData",{token,0x10000003,report,size,16});require(bool(r)&&r.eax==41,"report reusable while dispatching");
    g.set_u32(size,16);r=call("hid.dll","HidP_GetData",{0,list,size,descriptor,report+24,17});require(bool(r)&&r.eax==0x110000&&g.u32(size)==6,"neutral input data");
    require(g.u32(list)==10&&g.u32(list+4)==32768&&g.u32(list+12)==32767&&g.u32(list+44)==0,"neutral axes and null hat");
    p->windows.queue.clear();p->raw_input.packets.clear();
    p->host_xinput(std::nullopt);require(p->windows.queue.size()==1&&p->windows.queue[0].message==0xfe&&p->windows.queue[0].wparam==2,"disconnect notification");
    p->windows.queue.clear();p->host_xinput(HleXInputGamepad{});require(p->windows.queue.size()==2&&p->windows.queue[0].wparam==1,"arrival before report");
    r=call("hid.dll","HidP_GetCaps",{0,base});require(!r&&r.error.find("descriptor")!=std::string::npos,"unknown descriptor stops");
    std::cout<<"Raw input HLE: device enumeration/info, bounds, opaque descriptor, HID caps/data, neutral report, reusable packet and arrival/removal ordering; reference hardware identity remains assumed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
