#include "platform/mac/mac_xinput.hpp"
#include "platform/mac/mac_input.hpp"
#include <cmath>
namespace sadx::input::mac {
std::optional<HleXInputGamepad> xinput_sample(GCController* controller) noexcept{
    if(!controller)return std::nullopt;
    GCController* snapshot=[controller capture];
    const auto converted=input_mod_gamepad_state(snapshot);
    GCExtendedGamepad* pad=snapshot.extendedGamepad;
    if(!converted||!pad)return std::nullopt;
    const auto lt=pad.leftTrigger.value,rt=pad.rightTrigger.value;
    if(!std::isfinite(lt)||!std::isfinite(rt)||lt<0||lt>1||rt<0||rt>1)return std::nullopt;
    HleXInputGamepad out{};
    const unsigned masks[15]={0x1000,0x2000,0x4000,0x8000,0x20,0x400,0x10,0x40,0x80,0x100,0x200,1,2,4,8};
    for(unsigned k=0;k<15;++k)if(converted->state.buttons[k])out.buttons|=static_cast<std::uint16_t>(masks[k]);
    out.left_x=converted->state.axes[0];out.left_y=static_cast<std::int16_t>(-converted->state.axes[1]);
    out.right_x=converted->state.axes[2];out.right_y=static_cast<std::int16_t>(-converted->state.axes[3]);
    // Convert triggers directly from the snapshot to preserve the 30/31 threshold;
    // rescaling a truncated SDL axis would turn value 30/255 into 29.
    out.left_trigger=static_cast<std::uint8_t>(lt*255);out.right_trigger=static_cast<std::uint8_t>(rt*255);
    return out;
}
}
