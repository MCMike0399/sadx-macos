#include "hle/hle_raw_input.hpp"
#include "hle/hle_process.hpp"
#include <array>
#include <cstring>
#include <vector>
namespace sadx {
const char* HleRawInputState::device_name() noexcept{return "\\\\?\\HID#VID_045E&PID_028E&IG_00#SADX";}
std::optional<std::uint32_t> HleRawInputState::open(HleProcess& p,const std::string& name){
    if(name!=device_name())return std::nullopt;
    if(!p.xinput.pad){p.last_error=1167;return 0xffffffffU;}
    const auto handle=p.new_handle();open_handles.insert(handle);return handle;
}
bool HleRawInputState::close(std::uint32_t handle){return open_handles.erase(handle)!=0;}
namespace {
constexpr std::uint32_t bad=0xffffffffU;
void put(std::vector<std::uint8_t>& b,unsigned at,std::uint32_t v,unsigned size=4){
    for(unsigned k=0;k<size;++k)b.at(at+k)=static_cast<std::uint8_t>(v>>(8*k));
}
void identity(HleProcess& p){
    p.assume("Raw input: the synthetic wired user-0 pad exposes an Xbox-style HID identity (045e:028e, revision 0114, generic-desktop/gamepad usage, IG_00 path); reference device identity is uncaptured");
}
std::uint32_t arg(HleCall& c,unsigned i){return c.argument(i).value_or(0);}
}
void HleProcess::host_xinput(std::optional<HleXInputGamepad> pad){
    const auto connected=xinput.pad.has_value();const auto packet=xinput.packet;
    xinput.sample(pad);
    // Raw HID reports may repeat unchanged values; XInput's packet counter does not.
    // Sampling after an arrival also supplies state once SDL has opened the new joystick.
    if(packet!=xinput.packet||pad.has_value())raw_input.sample(*this,connected!=pad.has_value());
}
void HleRawInputState::sample(HleProcess& p,bool connection_changed){
    for(const auto& [usage,r]:registrations)if(usage==0x10005){
        const auto target=r.target?r.target:p.windows.focus;
        if(!target)continue;
        if(connection_changed&&(r.flags&0x2000))p.windows.queue.push_back({target,0xfe,p.xinput.pad?1U:2U,device,static_cast<std::uint32_t>(p.now_100ns/10000),0,0});
        if(p.xinput.pad)emit(p,target);
    }
}
void HleRawInputState::emit(HleProcess& p,std::uint32_t target){
    if(!p.xinput.pad)return;
    const auto& pad=*p.xinput.pad;
    std::vector<std::uint8_t> bytes(41);put(bytes,0,2);put(bytes,4,41);put(bytes,8,device);put(bytes,12,1);
    put(bytes,16,17);put(bytes,20,1); // one 17-byte report (zero ID followed by XINPUT_STATE wire fields).
    put(bytes,25,p.xinput.packet);put(bytes,29,pad.buttons,2);bytes[31]=pad.left_trigger;bytes[32]=pad.right_trigger;
    put(bytes,33,static_cast<std::uint16_t>(pad.left_x),2);put(bytes,35,static_cast<std::uint16_t>(pad.left_y),2);
    put(bytes,37,static_cast<std::uint16_t>(pad.right_x),2);put(bytes,39,static_cast<std::uint16_t>(pad.right_y),2);
    const auto token=p.new_handle();packets.emplace(token,std::move(bytes));
    p.windows.queue.push_back({target,0xff,1,token,static_cast<std::uint32_t>(p.now_100ns/10000),0,0});
}
void register_hle_raw_input(HleProcess& p){
    auto* P=&p;
    p.hle().add("USER32.dll","GetRawInputDeviceList",{12,3,[P](HleCall& c)->std::uint32_t{
        if(arg(c,2)!=8){P->last_error=87;return bad;}
        const auto capacity=c.guest.u32(arg(c,1));if(!capacity){c.guest.stop("GetRawInputDeviceList: unreadable count");return bad;}
        const std::uint32_t count=P->xinput.pad?1:0;
        if(!arg(c,0)){c.guest.set_u32(arg(c,1),count);return 0;}
        if(*capacity<count){c.guest.set_u32(arg(c,1),count);P->last_error=122;return bad;}
        if(count){identity(*P);const std::uint32_t entry[2]={HleRawInputState::device,2};
            if(!c.guest.write(arg(c,0),entry,sizeof entry)){c.guest.stop("GetRawInputDeviceList: unreadable output");return bad;}}
        c.guest.set_u32(arg(c,1),count);return count;}});
    p.hle().add("USER32.dll","RegisterRawInputDevices",{12,3,[P](HleCall& c)->std::uint32_t{
        if(arg(c,2)!=12){P->last_error=87;return 0;}
        for(unsigned k=0;k<arg(c,1);++k){
            std::uint32_t entry[3]={};if(!c.guest.read(arg(c,0)+12*k,entry,sizeof entry)){c.guest.stop("RegisterRawInputDevices: unreadable registration");return 0;}
            if(entry[2]&&!P->windows.find(entry[2])){P->last_error=1400;return 0;}
            const auto key=(entry[0]&0xffff)<<16|(entry[0]>>16);
            if(entry[1]&1){P->raw_input.registrations.erase(key);continue;}
            P->raw_input.registrations[key]={entry[1],entry[2]};
            if(key==0x10005&&P->xinput.pad){const auto target=entry[2]?entry[2]:P->windows.focus;if(target)P->raw_input.emit(*P,target);}
        }return 1;}});
    p.hle().add("USER32.dll","GetRawInputData",{20,5,[P](HleCall& c)->std::uint32_t{
        if(arg(c,4)!=16){P->last_error=87;return bad;}
        const auto packet=P->raw_input.packets.find(arg(c,0));
        if(packet==P->raw_input.packets.end()){P->last_error=6;return bad;}
        const auto command=arg(c,1);if(command!=0x10000003&&command!=0x10000005){c.guest.stop("GetRawInputData: unmodelled command");return bad;}
        const auto size=command==0x10000005?16U:static_cast<std::uint32_t>(packet->second.size());
        const auto capacity=c.guest.u32(arg(c,3));if(!capacity){c.guest.stop("GetRawInputData: unreadable size");return bad;}
        if(!arg(c,2)){c.guest.set_u32(arg(c,3),size);return 0;}
        if(*capacity<size){P->last_error=122;return bad;}
        if(!c.guest.write(arg(c,2),packet->second.data(),size)){c.guest.stop("GetRawInputData: unreadable output");return bad;}
        return size;}});
    const auto info=[P](bool wide){return [P,wide](HleCall& c)->std::uint32_t{
        if(arg(c,0)!=HleRawInputState::device||!P->xinput.pad){P->last_error=6;return bad;}
        const auto capacity=c.guest.u32(arg(c,3));if(!capacity){c.guest.stop("GetRawInputDeviceInfo: unreadable size");return bad;}
        identity(*P);std::vector<std::uint8_t> bytes;std::uint32_t size=0;
        switch(arg(c,1)){
        case 0x2000000b:bytes.resize(32);put(bytes,0,32);put(bytes,4,2);put(bytes,8,0x045e);put(bytes,12,0x028e);put(bytes,16,0x0114);put(bytes,20,1,2);put(bytes,22,5,2);size=32;break;
        case 0x20000007:{
            const auto* name=HleRawInputState::device_name();size=static_cast<std::uint32_t>(std::strlen(name)+1);
            for(unsigned k=0;k<size;++k){bytes.push_back(static_cast<std::uint8_t>(name[k]));if(wide)bytes.push_back(0);}break;}
        case 0x20000005:bytes.resize(8);put(bytes,0,0x53414458);put(bytes,4,HleRawInputState::device);size=8;break;
        default:c.guest.stop("GetRawInputDeviceInfo: unmodelled command");return bad;
        }
        if(!arg(c,2)){c.guest.set_u32(arg(c,3),size);return 0;}
        if(*capacity<size){c.guest.set_u32(arg(c,3),size);P->last_error=122;return bad;}
        if(!c.guest.write(arg(c,2),bytes.data(),bytes.size())){c.guest.stop("GetRawInputDeviceInfo: unreadable output");return bad;}
        return size;};};
    p.hle().add("USER32.dll","GetRawInputDeviceInfoA",{16,4,info(false)});
    p.hle().add("USER32.dll","GetRawInputDeviceInfoW",{16,4,info(true)});
    const auto descriptor=[](HleCall& c,unsigned i){
        const auto at=arg(c,i);
        if(c.guest.u32(at)!=0x53414458||c.guest.u32(at+4)!=HleRawInputState::device){c.guest.stop("HID parser: unknown preparsed descriptor");return false;}
        return true;};
    p.hle().add("hid.dll","HidP_MaxDataListLength",{8,2,[descriptor](HleCall& c)->std::uint32_t{
        if(!descriptor(c,1))return 0;
        if(arg(c,0)!=0){c.guest.stop("HidP_MaxDataListLength: only input reports are modelled");return 0;}
        return 16;}});
    p.hle().add("hid.dll","HidP_GetCaps",{8,2,[P,descriptor](HleCall& c)->std::uint32_t{
        if(!descriptor(c,0))return 0;
        P->assume("HID report: synthetic Xbox-style collection with 10 buttons, X/Y/Z/Rx/Ry and a hat; 16 data indices, report ID 0, 17-byte input reports; preparsed data is an opaque HLE token");
        std::vector<std::uint8_t> out(64);put(out,0,5,2);put(out,2,1,2);put(out,4,17,2);
        put(out,44,1,2);put(out,46,1,2);put(out,48,6,2);put(out,50,16,2);
        if(!c.guest.write(arg(c,1),out.data(),out.size()))c.guest.stop("HidP_GetCaps: unreadable output");
        return 0x110000;}});
    const auto caps=[descriptor](bool buttons){return [descriptor,buttons](HleCall& c)->std::uint32_t{
        if(!descriptor(c,3))return 0;
        if(arg(c,0)!=0){c.guest.stop("HID capability query: only input reports are modelled");return 0;}
        std::uint16_t count=0;if(!c.guest.read(arg(c,2),&count,2)){c.guest.stop("HID capability query: unreadable count");return 0;}
        const std::uint16_t needed=buttons?1:6;
        if(count<needed){c.guest.write(arg(c,2),&needed,2);return 0xc0110004;} // HIDP_STATUS_BUFFER_TOO_SMALL.
        std::vector<std::uint8_t> out(72*needed);
        if(buttons){
            put(out,0,9,2);put(out,4,2,2);put(out,8,5,2);put(out,10,1,2);out[12]=1;out[15]=1;
            put(out,56,1,2);put(out,58,10,2);put(out,68,0,2);put(out,70,9,2);
        }else{
            const unsigned usages[6]={0x30,0x31,0x32,0x33,0x34,0x39};
            for(unsigned k=0;k<6;++k){const auto at=72*k;put(out,at,1,2);put(out,at+4,2,2);put(out,at+8,5,2);put(out,at+10,1,2);out[at+15]=1;
                out[at+16]=k==5?1:0;put(out,at+18,k==5?4:16,2);put(out,at+20,1,2);
                put(out,at+40,k==5?1:0);put(out,at+44,k==5?8:65535);put(out,at+48,k==5?1:0);put(out,at+52,k==5?8:65535);
                put(out,at+56,usages[k],2);put(out,at+68,10+k,2);}
        }
        c.guest.write(arg(c,2),&needed,2);
        if(!c.guest.write(arg(c,1),out.data(),out.size()))c.guest.stop("HID capability query: unreadable output");
        return 0x110000;};};
    p.hle().add("hid.dll","HidP_GetButtonCaps",{16,4,caps(true)});
    p.hle().add("hid.dll","HidP_GetValueCaps",{16,4,caps(false)});
    p.hle().add("hid.dll","HidP_GetData",{24,6,[descriptor](HleCall& c)->std::uint32_t{
        if(!descriptor(c,3))return 0;
        if(arg(c,0)!=0||arg(c,5)!=17){c.guest.stop("HidP_GetData: unsupported report type/length");return 0;}
        std::array<std::uint8_t,17> report{};
        if(!c.guest.read(arg(c,4),report.data(),report.size())||report[0]!=0){c.guest.stop("HidP_GetData: invalid input report");return 0;}
        const auto word=[&](unsigned at){return std::uint32_t(report[at])|(std::uint32_t(report[at+1])<<8);};
        const auto buttons=word(5);const unsigned masks[10]={0x1000,0x2000,0x4000,0x8000,0x100,0x200,0x20,0x10,0x40,0x80};
        std::vector<std::uint8_t> out;
        const auto item=[&](unsigned index,std::uint32_t value){const auto at=static_cast<unsigned>(out.size());out.resize(at+8);put(out,at,index,2);put(out,at+4,value);};
        for(unsigned k=0;k<10;++k)if(buttons&masks[k])item(k,1);
        item(10,static_cast<std::int16_t>(word(9))+32768);
        item(11,32767-static_cast<std::int16_t>(word(11)));
        item(12,static_cast<std::uint32_t>(32768+(int(report[7])-int(report[8]))*32767/255));
        item(13,static_cast<std::int16_t>(word(13))+32768);
        item(14,32767-static_cast<std::int16_t>(word(15)));
        unsigned hat=0;
        switch(buttons&15){case 1:hat=1;break;case 9:hat=2;break;case 8:hat=3;break;case 10:hat=4;break;case 2:hat=5;break;case 6:hat=6;break;case 4:hat=7;break;case 5:hat=8;break;default:break;}
        item(15,hat);
        const auto capacity=c.guest.u32(arg(c,2));if(!capacity){c.guest.stop("HidP_GetData: unreadable data count");return 0;}
        const auto count=static_cast<std::uint32_t>(out.size()/8);
        if(*capacity<count){c.guest.set_u32(arg(c,2),count);return 0xc0110004;}
        if(!c.guest.write(arg(c,1),out.data(),out.size())){c.guest.stop("HidP_GetData: unreadable output");return 0;}
        c.guest.set_u32(arg(c,2),count);return 0x110000;}});
    const auto hid_string=[P](const char* value){return [P,value](HleCall& c)->std::uint32_t{
        if(!P->raw_input.open_handles.count(arg(c,0))){P->last_error=6;return 0;}
        const auto size=(std::strlen(value)+1)*2;if(arg(c,2)<size){P->last_error=122;return 0;}
        std::vector<std::uint8_t> bytes;for(std::size_t i=0;i<std::strlen(value)+1;++i){bytes.push_back(static_cast<std::uint8_t>(value[i]));bytes.push_back(0);}
        if(!c.guest.write(arg(c,1),bytes.data(),bytes.size()))c.guest.stop("HidD string: unreadable output");
        return 1;};};
    p.hle().add("hid.dll","HidD_GetManufacturerString",{12,3,hid_string("sadx-macos")});
    p.hle().add("hid.dll","HidD_GetProductString",{12,3,hid_string("Virtual XInput Controller")});
}
} // namespace sadx
