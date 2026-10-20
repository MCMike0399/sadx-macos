#include "platform/mac/hid_switch_controller.hpp"
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <array>
#include <dlfcn.h>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool ok, const std::string& why) { if (!ok) throw std::runtime_error(why); }
template<class Function> Function symbol(void* library, const char* name) {
    auto function = reinterpret_cast<Function>(dlsym(library,name));
    require(function != nullptr,name);
    return function;
}
bool SDLCALL received_rumble(void* context, Uint16 low, Uint16 high) {
    static_cast<std::vector<std::array<Uint16,2>>*>(context)->push_back({low,high});
    return true;
}
}

// Uses SDL's actual virtual joystick driver and real dynamic library, not a mocked backend.
int main(int argc, char** argv) { try {
    require(argc == 2,"usage: mac-switch-backend-test /path/to/native/libSDL3.0.dylib");
    using namespace sadx::input::mac;
    void* library = dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    require(library != nullptr,"native SDL3 dependency");
    const auto attach = symbol<decltype(&SDL_AttachVirtualJoystick)>(library,"SDL_AttachVirtualJoystick");
    const auto detach = symbol<decltype(&SDL_DetachVirtualJoystick)>(library,"SDL_DetachVirtualJoystick");
    const auto open = symbol<decltype(&SDL_OpenJoystick)>(library,"SDL_OpenJoystick");
    const auto close = symbol<decltype(&SDL_CloseJoystick)>(library,"SDL_CloseJoystick");
    const auto axis = symbol<decltype(&SDL_SetJoystickVirtualAxis)>(library,"SDL_SetJoystickVirtualAxis");
    const auto button = symbol<decltype(&SDL_SetJoystickVirtualButton)>(library,"SDL_SetJoystickVirtualButton");
    const auto mapping = symbol<decltype(&SDL_SetGamepadMapping)>(library,"SDL_SetGamepadMapping");
    const auto joystick_events = symbol<decltype(&SDL_JoystickEventsEnabled)>(library,"SDL_JoystickEventsEnabled");
    const auto gamepad_events = symbol<decltype(&SDL_GamepadEventsEnabled)>(library,"SDL_GamepadEventsEnabled");
    const auto hint = symbol<decltype(&SDL_SetHint)>(library,"SDL_SetHint");
    // Keep this automated API/driver test independent of the user's attached physical devices.
    // The USB protocol and real device gestures have their separate recorded qualification.
    require(hint(SDL_HINT_JOYSTICK_HIDAPI,"0") && hint(SDL_HINT_JOYSTICK_IOKIT,"0") &&
            hint(SDL_HINT_JOYSTICK_MFI,"0"),"isolate SDL virtual driver");
    std::string error;
    HidSwitchController controller;
    require(controller.initialize(argv[1],error),error);
    require(!joystick_events() && !gamepad_events(),"polling must not accumulate unused input events");
    std::vector<std::array<Uint16,2>> motors;
    SDL_VirtualJoystickDesc description{};
    SDL_INIT_INTERFACE(&description);
    description.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    description.vendor_id = 0x057e;
    description.product_id = 0x2069;
    description.naxes = 6;
    description.nbuttons = 15;
    description.axis_mask = 63;
    description.button_mask = 32767;
    description.name = "Native Switch mapping qualification";
    description.userdata = &motors;
    description.Rumble = received_rumble;
    const auto create = [&]() {
        const auto id = attach(&description);
        require(id != 0,"virtual attach");
        // State supplied here is positional SDL joystick input. Nintendo labels are independent
        // of those positions, as they are for the actual SDL Switch2 hardware driver.
        require(mapping(id,"ff0000007e0500006920000000007600,Native Switch mapping qualification,"
                           "a:b0,b:b1,x:b2,y:b3,back:b4,guide:b5,start:b6,leftstick:b7,rightstick:b8,"
                           "leftshoulder:b9,rightshoulder:b10,dpup:b11,dpdown:b12,dpleft:b13,dpright:b14,"
                           "leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,"
                           "type:nintendoswitchpro,"),"virtual mapping");
        auto* joystick = open(id);
        require(joystick != nullptr,"virtual joystick open");
        require(axis(joystick,4,-32768) && axis(joystick,5,-32768),"neutral triggers");
        return std::pair<SDL_JoystickID,SDL_Joystick*>{id,joystick};
    };
    auto [first_id,joystick] = create();
    auto sample = controller.poll(error);
    require(sample.has_value() && error.empty(),"poll newly attached device: " + error);
    require(sample->instance_id == first_id && sample->rumble_supported,"identity and actual capability");
    constexpr unsigned destinations[15] = {1,0,3,2,4,5,6,7,8,9,10,11,12,13,14};
    for (unsigned index = 0; index < 15; ++index) {
        require(button(joystick,index,true),"virtual button press");
        sample = controller.poll(error);
        require(sample.has_value() && error.empty(),error);
        for (unsigned target = 0; target < 15; ++target) {
            require(sample->positional_buttons[target] == (target == index),"raw SDL position");
            require(sample->state.buttons[target] == (target == destinations[index]),"printed-label mapping");
        }
        require(button(joystick,index,false),"virtual button release");
        sample = controller.poll(error);
        require(sample && !sample->state.buttons[destinations[index]],"button release state");
    }
    const std::array<Sint16,6> expected = {-32768,32767,16384,-8192,0,32767};
    for (unsigned index = 0; index < expected.size(); ++index)
        require(axis(joystick,index,index >= 4 ? (expected[index] == 0 ? -32768 : 32767) : expected[index]),"virtual axis");
    sample = controller.poll(error);
    require(sample && sample->state.axes == expected,"manual polling preserves unprocessed SDL axes");
    require(controller.rumble(0x1234,0xabcd,100,error),"native rumble API");
    require(!motors.empty() && motors.back() == std::array<Uint16,2>{0x1234,0xabcd},"rumble reaches actual SDL driver");
    require(controller.rumble(0,0,0,error) && motors.back() == std::array<Uint16,2>{0,0},"rumble stop reaches driver");
    close(joystick);
    require(detach(first_id),"virtual detach");
    require(!controller.poll(error) && error.empty(),"detach clears state without a fixture fact");
    require(!controller.rumble(1,1,100,error) && !error.empty(),"disconnected rumble rejected");
    auto [second_id,reconnected] = create();
    sample = controller.poll(error);
    require(sample && sample->instance_id == second_id && second_id != first_id,"reconnect new device identity");
    require(sample->state.buttons == std::array<bool,15>{},"reconnect has no stale pressed buttons");
    close(reconnected);
    require(detach(second_id),"final virtual detach");
    controller.shutdown();
    require(controller.initialize(argv[1],error),"backend can initialize after cleanup");
    controller.shutdown();
    dlclose(library);
    std::cout << "Native SDL backend: real virtual-driver 15 positions/labels, axes, polling without "
                 "events, rumble callback, disconnect/reconnect and cleanup pass\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
} }
