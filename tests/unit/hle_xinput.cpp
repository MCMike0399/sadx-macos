#include "hle/hle_process.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
namespace {
using namespace sadx;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
}
int main(){try{
    std::string error;auto p=HleProcess::create(".","build/xinput-overlay",&error);require(p!=nullptr,"create process");
    auto& g=p->guest();const auto buffer=p->memory().allocate(32);require(buffer.has_value(),"buffer");
    const auto call=[&](const char* dll,const char* name,std::initializer_list<std::uint32_t> args){
        const auto trap=p->hle().trap(g,dll,name,&error);require(trap.has_value(),"trap");return g.call(*trap,args);};
    const auto word=[&](unsigned at){const auto v=g.u32(*buffer+at);require(v.has_value(),"wire word");return *v;};
    p->last_error=0x1234;
    for(const auto* dll:{"XInput1_4.dll","XInput1_3.dll","XInput9_1_0.dll"}){
        std::array<std::uint8_t,32> sentinel;sentinel.fill(0xa5);g.write(*buffer,sentinel.data(),sentinel.size());
        auto r=call(dll,"XInputGetState",{0,*buffer+4});require(bool(r)&&r.eax==1167,"disconnected");
        require(word(4)==0xa5a5a5a5&&p->last_error==0x1234,"disconnected output/LastError untouched");
        HleXInputGamepad pad{0x1411,30,255,-32768,32767,-1,0};p->xinput.sample(pad);
        const auto packet=p->xinput.packet;p->xinput.sample(pad);require(p->xinput.packet==packet,"identical sample does not change packet");
        r=call(dll,"XInputGetState",{0,*buffer+4});require(bool(r)&&r.eax==0,"connected state");
        require(word(4)==packet&&word(8)==0xff1e1011&&word(12)==0x7fff8000&&word(16)==0x0000ffff,"standard state layout, Guide excluded, signed axis boundaries");
        require(word(0)==0xa5a5a5a5&&word(20)==0xa5a5a5a5,"state output bounds");
        r=call(dll,"#100",{0,*buffer+4});require(bool(r)&&r.eax==0&&word(8)==0xff1e1411,"GetStateEx includes Guide");
        require(p->xinput.packet==packet,"polling does not change packet");
        pad.left_trigger=31;p->xinput.sample(pad);require(p->xinput.packet==packet+1,"trigger change increments packet");
        r=call(dll,"XInputGetCapabilities",{0,1,*buffer+4});require(bool(r)&&r.eax==0,"capabilities");
        require(word(4)==0x00000101&&word(8)==0xfffff3ff&&word(12)==0xffffffff&&word(16)==0xffffffff&&word(20)==0,"20-byte caps, wired gamepad, resolution, no actuators");
        r=call(dll,"XInputGetState",{1,*buffer+4});require(bool(r)&&r.eax==1167,"only user 0 exists");
        r=call(dll,"XInputEnable",{0});require(bool(r),"disable");
        r=call(dll,"XInputGetState",{0,*buffer+4});require(bool(r)&&word(8)==0&&word(12)==0&&word(16)==0,"disabled neutral state");
        r=call(dll,"XInputEnable",{1});require(bool(r),"enable");
        r=call(dll,"XInputGetState",{0,*buffer+4});require(bool(r)&&word(8)==0xff1f1011,"reenabled host state");
        r=call(dll,"XInputSetState",{0,*buffer});require(bool(r)&&r.eax==50,"unsupported vibration explicitly reported");
        r=call(dll,"XInputGetBatteryInformation",{0,0,*buffer});require(bool(r)&&(word(0)&0xff)==1,"wired battery type");
        p->xinput.sample(std::nullopt);require(p->xinput.packet==packet+2,"disconnect changes packet");
        r=call(dll,"XInputGetCapabilities",{0,1,*buffer});require(bool(r)&&r.eax==1167,"disconnected caps");
    }
    require(p->last_error==0x1234,"XInput does not use SetLastError");
    p->xinput.sample(HleXInputGamepad{});
    const auto r=call("XInput1_4.dll","XInputGetState",{0,0});require(!r&&r.error.find("output buffer")!=std::string::npos,"invalid buffer stops explicitly");
    std::cout<<"XInput HLE: three DLL aliases, connected/disconnected users, wire layouts/bounds, Guide, packet changes, enable/disable, wired battery, explicit unsupported vibration/invalid buffer; pinned SDL enumeration still needs the raw-input device facade\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
