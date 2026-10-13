// DirectInput 8 for the guest: the IDirectInput8 object (created by DirectInput8Create or CoCreateInstance of
// CLSID_DirectInput8). The guest machine has no game controllers: device enumeration finds none. Keyboard
// and mouse devices (sonic.exe's) are added when the guest reaches CreateDevice.
#include "hle/hle_com.hpp"
#include <memory>
namespace sadx {
namespace {
constexpr const char* clsid_direct_input8="25e609e4-b259-11cf-bfc7-444553540000";
}
void register_hle_dinput(HleProcess& p){
    auto* P=&p;
    auto direct_input=std::make_shared<std::unique_ptr<HleComInterface>>();
    const auto interface_for=[P,direct_input]()->HleComInterface&{
        if(!*direct_input)*direct_input=std::make_unique<HleComInterface>(*P,"IDirectInput8",std::vector<HleComMethod>{
            com_query_interface_stop("IDirectInput8"),com_add_ref(),com_release(),
            {"CreateDevice",16,[](HleCall& c)->std::uint32_t{c.guest.stop("IDirectInput8::CreateDevice("+guid_text(c.guest,c.argument(1).value_or(0))+") is not modelled yet");return 0x80004005U;}},
            {"EnumDevices",20,[](HleCall&)->std::uint32_t{return 0;}}, // DI_OK, no device reported
            {"GetDeviceStatus",8,[](HleCall&)->std::uint32_t{return 0x80040154U;}}, // DIERR_DEVICENOTREG: no such device
            {"RunControlPanel",12,[](HleCall& c)->std::uint32_t{c.guest.stop("IDirectInput8::RunControlPanel");return 0;}},
            {"Initialize",12,[](HleCall&)->std::uint32_t{return 0;}},
            {"FindDevice",16,[](HleCall&)->std::uint32_t{return 0x80040154U;}},
            {"EnumDevicesBySemantics",24,[](HleCall&)->std::uint32_t{return 0;}},
            {"ConfigureDevices",20,[](HleCall& c)->std::uint32_t{c.guest.stop("IDirectInput8::ConfigureDevices");return 0;}}});
        return **direct_input;};
    p.hle().add("DINPUT8.dll","DirectInput8Create",{20,5,[interface_for](HleCall& c)->std::uint32_t{
        const auto object=interface_for().create(16);if(!object)return 0x8007000eU; // E_OUTOFMEMORY
        c.guest.set_u32(c.argument(3).value_or(0),*object);return 0;}});
    // CoCreateInstance(CLSID_DirectInput8, …, IID_IDirectInput8A/W, &ppv): the same object (the caller then calls Initialize).
    const auto previous=*p.hle().find("ole32.dll","CoCreateInstance");
    p.hle().add("ole32.dll","CoCreateInstance",{20,5,[interface_for,previous](HleCall& c)->std::uint32_t{
        if(guid_text(c.guest,c.argument(0).value_or(0))!=clsid_direct_input8)return previous.body(c);
        const auto object=interface_for().create(16);if(!object)return 0x8007000eU;
        c.guest.set_u32(c.argument(4).value_or(0),*object);return 0;}});
}
} // namespace sadx
