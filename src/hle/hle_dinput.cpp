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
    // IDirectInputDevice8 for the system mouse and keyboard: no input arrives yet (the host feeds it later),
    // so state reads are all zero and buffered reads return no events.
    auto device_if=std::make_shared<std::unique_ptr<HleComInterface>>();
    const auto device_interface=[P,device_if]()->HleComInterface&{
        if(*device_if)return **device_if;
        const auto ok=[](HleCall&)->std::uint32_t{return 0;};
        const auto stop=[](const char* name){return [name](HleCall& c)->std::uint32_t{c.guest.stop(std::string("IDirectInputDevice8::")+name+" is not modelled yet");return 0x80004005U;};};
        std::vector<HleComMethod> m={com_query_interface_stop("IDirectInputDevice8"),com_add_ref(),com_release(),
            {"GetCapabilities",8,stop("GetCapabilities")},{"EnumObjects",16,stop("EnumObjects")},{"GetProperty",12,stop("GetProperty")},{"SetProperty",12,ok},
            {"Acquire",4,ok},{"Unacquire",4,ok},
            {"GetDeviceState",12,[](HleCall& c)->std::uint32_t{const std::vector<std::uint8_t> z(c.argument(1).value_or(0),0);c.guest.write(c.argument(2).value_or(0),z.data(),z.size());return 0;}},
            {"GetDeviceData",20,[](HleCall& c)->std::uint32_t{c.guest.set_u32(c.argument(3).value_or(0),0);return 0;}}, // *pdwInOut = 0 events
            {"SetDataFormat",8,ok},{"SetEventNotification",8,ok},{"SetCooperativeLevel",12,ok},{"GetObjectInfo",16,stop("GetObjectInfo")},
            {"GetDeviceInfo",8,stop("GetDeviceInfo")},{"RunControlPanel",12,stop("RunControlPanel")},{"Initialize",16,ok},{"CreateEffect",20,stop("CreateEffect")},
            {"EnumEffects",16,ok},{"GetEffectInfo",12,stop("GetEffectInfo")},{"GetForceFeedbackState",8,stop("GetForceFeedbackState")},
            {"SendForceFeedbackCommand",8,stop("SendForceFeedbackCommand")},{"EnumCreatedEffectObjects",16,ok},{"Escape",8,stop("Escape")},{"Poll",4,ok},
            {"SendDeviceData",20,stop("SendDeviceData")},{"EnumEffectsInFile",20,stop("EnumEffectsInFile")},{"WriteEffectToFile",20,stop("WriteEffectToFile")},
            {"BuildActionMap",16,stop("BuildActionMap")},{"SetActionMap",16,stop("SetActionMap")},{"GetImageInfo",8,stop("GetImageInfo")}};
        *device_if=std::make_unique<HleComInterface>(*P,"IDirectInputDevice8",std::move(m));return **device_if;};
    auto direct_input=std::make_shared<std::unique_ptr<HleComInterface>>();
    const auto interface_for=[P,direct_input,device_interface]()->HleComInterface&{
        if(!*direct_input)*direct_input=std::make_unique<HleComInterface>(*P,"IDirectInput8",std::vector<HleComMethod>{
            com_query_interface_stop("IDirectInput8"),com_add_ref(),com_release(),
            {"CreateDevice",16,[P,device_interface](HleCall& c)->std::uint32_t{
                const auto guid=guid_text(c.guest,c.argument(1).value_or(0));
                if(guid!="6f1d2b60-d5a0-11cf-bfc7-444553540000"&&guid!="6f1d2b61-d5a0-11cf-bfc7-444553540000"){c.guest.stop("IDirectInput8::CreateDevice("+guid+") is not modelled");return 0x80004005U;}
                P->assume("DirectInput system mouse/keyboard devices exist but report no input (host input is not fed to the guest yet)");
                const auto object=device_interface().create(16);if(!object)return 0x8007000eU;c.guest.set_u32(c.argument(2).value_or(0),*object);return 0;}},
            {"EnumDevices",20,[](HleCall&)->std::uint32_t{return 0;}}, // DI_OK, no device reported
            {"GetDeviceStatus",8,[](HleCall&)->std::uint32_t{return 0x80040154U;}}, // DIERR_DEVICENOTREG: no such device
            {"RunControlPanel",12,[](HleCall& c)->std::uint32_t{c.guest.stop("IDirectInput8::RunControlPanel");return 0;}},
            {"Initialize",12,[](HleCall&)->std::uint32_t{return 0;}},
            {"FindDevice",16,[](HleCall&)->std::uint32_t{return 0x80040154U;}},
            {"EnumDevicesBySemantics",24,[](HleCall&)->std::uint32_t{return 0;}},
            {"ConfigureDevices",20,[](HleCall& c)->std::uint32_t{c.guest.stop("IDirectInput8::ConfigureDevices");return 0;}}});
        return **direct_input;};
    // DirectSoundCreate8 (DSOUND #11): no audio backend yet, as in the native runtime (410510/44de80).
    p.hle().add("DSOUND.dll","#11",{12,3,[P](HleCall&)->std::uint32_t{
        P->assume("DirectSoundCreate8 reports DSERR_NODRIVER (no audio backend yet; the Windows reference has sound), as the native runtime does");return 0x88780078U;}});
    p.hle().add("DSOUND.dll","DirectSoundCreate8",{12,3,[P](HleCall&)->std::uint32_t{
        P->assume("DirectSoundCreate8 reports DSERR_NODRIVER (no audio backend yet; the Windows reference has sound), as the native runtime does");return 0x88780078U;}});
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
