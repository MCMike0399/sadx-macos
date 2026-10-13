#include "platform/mac/mac_xinput.hpp"
#include <iostream>
#include <stdexcept>
namespace {void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(){@autoreleasepool{try{
    require(!sadx::input::mac::xinput_sample(nil),"disconnected");
    require(!sadx::input::mac::xinput_sample([GCController controllerWithMicroGamepad]),"unsupported profile");
    GCController* c=[GCController controllerWithExtendedGamepad];
    [c.extendedGamepad.leftThumbstick setValueForXAxis:-1 yAxis:0.5f];
    [c.extendedGamepad.rightThumbstick setValueForXAxis:0.25f yAxis:-1];
    [c.extendedGamepad.buttonA setValue:1];[c.extendedGamepad.buttonMenu setValue:1];
    [c.extendedGamepad.dpad setValueForXAxis:-1 yAxis:0];
    [c.extendedGamepad.leftTrigger setValue:30.0f/255];[c.extendedGamepad.rightTrigger setValue:1];
    auto p=sadx::input::mac::xinput_sample(c);require(p.has_value(),"extended profile");
    require((p->buttons&0x1014)==0x1014,"A, Start, left D-pad");
    require(p->left_x==-32767&&p->left_y==16383&&p->right_x==8191&&p->right_y==-32767,"raw XInput axis signs/quantization");
    require(p->left_trigger==30&&p->right_trigger==255,"trigger conversion preserves threshold");
    [c.extendedGamepad.leftTrigger setValue:31.0f/255];
    p=sadx::input::mac::xinput_sample(c);require(p&&p->left_trigger==31,"threshold crossing");
    std::cout<<"Mac XInput: stable extended-profile sample, buttons, raw-axis signs/limits and trigger 30/31 preserved; disconnected/unsupported profiles explicit\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}}
