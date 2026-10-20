#include "hle/hle_xinput.hpp"
#include "hle/hle_process.hpp"
#include <array>
namespace sadx {
bool HleXInputGamepad::operator==(const HleXInputGamepad& o)const noexcept{
    return buttons==o.buttons&&left_trigger==o.left_trigger&&right_trigger==o.right_trigger&&
        left_x==o.left_x&&left_y==o.left_y&&right_x==o.right_x&&right_y==o.right_y;
}
void HleXInputState::sample(std::optional<HleXInputGamepad> next){
    if (pad.has_value() != next.has_value()) requested_vibration = {};
    if(!(next==pad)){++packet;pad=next;}
}
namespace {
template<std::size_t N>void put(std::array<std::uint8_t,N>& b,unsigned at,std::uint32_t value,unsigned size){
    for(unsigned i=0;i<size;++i)b[at+i]=static_cast<std::uint8_t>(value>>(8*i));
}
template<std::size_t N>void gamepad(std::array<std::uint8_t,N>& b,unsigned at,const HleXInputGamepad& p,bool guide){
    put(b,at,p.buttons&(guide?0xf7ff:0xf3ff),2);
    b[at+2]=p.left_trigger;b[at+3]=p.right_trigger;
    put(b,at+4,static_cast<std::uint16_t>(p.left_x),2);put(b,at+6,static_cast<std::uint16_t>(p.left_y),2);
    put(b,at+8,static_cast<std::uint16_t>(p.right_x),2);put(b,at+10,static_cast<std::uint16_t>(p.right_y),2);
}
bool connected(HleProcess& p,HleCall& c){
    const auto user=c.argument(0);
    if(!user||*user>3){c.guest.stop("XInput: user index outside 0..3");return false;}
    return *user==0&&p.xinput.pad.has_value();
}
bool write(HleCall& c,unsigned argument,const void* data,std::size_t size,const char* api){
    const auto at=c.argument(argument);
    if(!at||!c.guest.write(*at,data,size)){c.guest.stop(std::string(api)+": unreadable output buffer");return false;}
    return true;
}
}
void register_hle_xinput(HleProcess& p){
    auto* P=&p;
    for(const auto* dll:{"XInput1_4.dll","XInput1_3.dll","XInput9_1_0.dll"}){
        const auto state=[P](bool guide){return [P,guide](HleCall& c)->std::uint32_t{
            if(!connected(*P,c))return 1167;
            P->assume("XInput: one synthetic wired gamepad at user 0; deterministic packet number increments on host sample changes, not polls (reference device identity and packet policy were not captured)");
            std::array<std::uint8_t,16> out{};put(out,0,P->xinput.packet,4);
            gamepad(out,4,P->xinput.enabled?*P->xinput.pad:HleXInputGamepad{},guide);
            write(c,1,out.data(),out.size(),"XInputGetState");return 0;};};
        p.hle().add(dll,"XInputGetState",{8,2,state(false)});
        p.hle().add(dll,"#100",{8,2,state(true)}); // SDL's loaded XInputGetStateEx: same layout, with Guide.
        p.hle().add(dll,"XInputGetCapabilities",{12,3,[P](HleCall& c)->std::uint32_t{
            if(!connected(*P,c))return 1167;
            const auto flags=c.argument(1);if(!flags||(*flags!=0&&*flags!=1)){c.guest.stop("XInputGetCapabilities: unsupported flags");return 87;}
            P->assume("XInput capabilities: synthetic wired gamepad, standard buttons, 8-bit triggers and 16-bit sticks; voice unavailable; vibration is present only with a supported native owner");
            std::array<std::uint8_t,20> out{};out[0]=1;out[1]=1;
            put(out,4,0xf3ff,2);out[6]=out[7]=0xff;for(unsigned k=8;k<16;++k)out[k]=0xff;
            if (P->xinput.vibration_supported && P->xinput.set_vibration) {
                put(out,2,1,2); // XINPUT_CAPS_FFB_SUPPORTED
                put(out,16,65535,2);put(out,18,65535,2);
            }
            write(c,2,out.data(),out.size(),"XInputGetCapabilities");return 0;}});
        p.hle().add(dll,"XInputEnable",{4,1,[P](HleCall& c)->std::uint32_t{
            P->xinput.enabled=c.argument(0).value_or(0)!=0;
            // XInputEnable stops vibration while disabled, and restores the last registered
            // request when enabled (Microsoft XInputEnable API contract).
            if (P->xinput.pad && P->xinput.vibration_supported && P->xinput.set_vibration) {
                const auto request=P->xinput.enabled?P->xinput.requested_vibration:std::array<std::uint16_t,2>{};
                P->xinput.vibration_status=P->xinput.set_vibration(request[0],request[1]);
            }
            return 0;}});
        p.hle().add(dll,"XInputSetState",{8,2,[P](HleCall& c)->std::uint32_t{
            if(!connected(*P,c))return 1167;
            if (!P->xinput.vibration_supported || !P->xinput.set_vibration) {
                P->assume("XInputSetState: selected controller has no native vibration owner; requests return ERROR_NOT_SUPPORTED");
                return 50;
            }
            const auto at=c.argument(1);
            std::array<std::uint8_t,4> bytes{};
            if (!at || !c.guest.read(*at,bytes.data(),bytes.size())) {
                c.guest.stop("XInputSetState: unreadable vibration buffer");
                return 87;
            }
            const std::array<std::uint16_t,2> request={
                static_cast<std::uint16_t>(bytes[0]|(bytes[1]<<8)),
                static_cast<std::uint16_t>(bytes[2]|(bytes[3]<<8))};
            ++P->xinput.vibration_requests;
            if (!P->xinput.enabled) {
                P->xinput.requested_vibration=request;
                P->xinput.vibration_status=0;
                return 0;
            }
            const auto status=P->xinput.set_vibration(request[0],request[1]);
            P->xinput.vibration_status=status;
            if (status==0) P->xinput.requested_vibration=request;
            return status;}});
        p.hle().add(dll,"XInputGetBatteryInformation",{12,3,[P](HleCall& c)->std::uint32_t{
            if(!connected(*P,c))return 1167;
            if(c.argument(1)!=0){c.guest.stop("XInputGetBatteryInformation: only the gamepad is modelled");return 50;}
            const std::uint8_t out[2]={1,3}; // BATTERY_TYPE_WIRED, BATTERY_LEVEL_FULL.
            write(c,2,out,sizeof out,"XInputGetBatteryInformation");return 0;}});
        p.hle().add(dll,"XInputGetKeystroke",{12,3,[P](HleCall& c)->std::uint32_t{
            if(!connected(*P,c))return 1167;c.guest.stop("XInputGetKeystroke: connected-device keystrokes are not modelled");return 50;}});
        p.hle().add(dll,"#108",{16,4,[P](HleCall& c)->std::uint32_t{
            if(!connected(*P,c))return 1167;c.guest.stop("XInputGetCapabilitiesEx: extended device identity is not modelled");return 50;}});
    }
}
} // namespace sadx
