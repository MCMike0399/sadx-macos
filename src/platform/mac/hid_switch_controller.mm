#include "platform/mac/hid_switch_controller.hpp"
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <array>
#include <dlfcn.h>

namespace sadx::input::mac {
struct HidSwitchController::Impl {
    void* library = nullptr;
    SDL_Gamepad* gamepad = nullptr;
    bool initialized = false;
    bool prior_joystick_events = false;
    bool prior_gamepad_events = false;
    decltype(&SDL_InitSubSystem) init = nullptr;
    decltype(&SDL_QuitSubSystem) quit = nullptr;
    decltype(&SDL_GetVersion) version = nullptr;
    decltype(&SDL_GetError) error = nullptr;
    decltype(&SDL_GetGamepads) gamepads = nullptr;
    decltype(&SDL_GetGamepadVendorForID) vendor = nullptr;
    decltype(&SDL_GetGamepadProductForID) product = nullptr;
    decltype(&SDL_OpenGamepad) open = nullptr;
    decltype(&SDL_CloseGamepad) close = nullptr;
    decltype(&SDL_UpdateGamepads) update = nullptr;
    decltype(&SDL_GamepadConnected) connected = nullptr;
    decltype(&SDL_GetGamepadID) id = nullptr;
    decltype(&SDL_GetGamepadAxis) axis = nullptr;
    decltype(&SDL_GetGamepadButton) button = nullptr;
    decltype(&SDL_GetGamepadButtonLabel) label = nullptr;
    decltype(&SDL_GetGamepadProperties) properties = nullptr;
    decltype(&SDL_GetBooleanProperty) boolean_property = nullptr;
    decltype(&SDL_RumbleGamepad) rumble = nullptr;
    decltype(&SDL_free) free = nullptr;
    decltype(&SDL_JoystickEventsEnabled) joystick_events = nullptr;
    decltype(&SDL_GamepadEventsEnabled) gamepad_events = nullptr;
    decltype(&SDL_SetJoystickEventsEnabled) set_joystick_events = nullptr;
    decltype(&SDL_SetGamepadEventsEnabled) set_gamepad_events = nullptr;
};

HidSwitchController::HidSwitchController() : impl_(std::make_unique<Impl>()) {}
HidSwitchController::~HidSwitchController() { shutdown(); }

// [platform:input] SDL owns USB initialization, factory calibration, reports and HD rumble.
bool HidSwitchController::initialize(const std::string& library_path, std::string& error) {
    error.clear();
    if (impl_->library) {
        error = "native Switch controller backend already initialized";
        return false;
    }
    impl_->library = dlopen(library_path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!impl_->library) {
        error = dlerror();
        return false;
    }
#define LOAD_SDL(member, symbol) \
    impl_->member = reinterpret_cast<decltype(impl_->member)>(dlsym(impl_->library, #symbol)); \
    if (!impl_->member) { \
        error = "native controller dependency missing " #symbol; \
        shutdown(); \
        return false; \
    }
    LOAD_SDL(init, SDL_InitSubSystem)
    LOAD_SDL(quit, SDL_QuitSubSystem)
    LOAD_SDL(version, SDL_GetVersion)
    LOAD_SDL(error, SDL_GetError)
    LOAD_SDL(gamepads, SDL_GetGamepads)
    LOAD_SDL(vendor, SDL_GetGamepadVendorForID)
    LOAD_SDL(product, SDL_GetGamepadProductForID)
    LOAD_SDL(open, SDL_OpenGamepad)
    LOAD_SDL(close, SDL_CloseGamepad)
    LOAD_SDL(update, SDL_UpdateGamepads)
    LOAD_SDL(connected, SDL_GamepadConnected)
    LOAD_SDL(id, SDL_GetGamepadID)
    LOAD_SDL(axis, SDL_GetGamepadAxis)
    LOAD_SDL(button, SDL_GetGamepadButton)
    LOAD_SDL(label, SDL_GetGamepadButtonLabel)
    LOAD_SDL(properties, SDL_GetGamepadProperties)
    LOAD_SDL(boolean_property, SDL_GetBooleanProperty)
    LOAD_SDL(rumble, SDL_RumbleGamepad)
    LOAD_SDL(free, SDL_free)
    LOAD_SDL(joystick_events, SDL_JoystickEventsEnabled)
    LOAD_SDL(gamepad_events, SDL_GamepadEventsEnabled)
    LOAD_SDL(set_joystick_events, SDL_SetJoystickEventsEnabled)
    LOAD_SDL(set_gamepad_events, SDL_SetGamepadEventsEnabled)
#undef LOAD_SDL
    if (impl_->version() < SDL_VERSIONNUM(3, 4, 2)) {
        error = "native controller dependency requires SDL3 3.4.2 or newer";
        shutdown();
        return false;
    }
    if (!impl_->init(SDL_INIT_GAMEPAD)) {
        error = impl_->error();
        shutdown();
        return false;
    }
    impl_->initialized = true;
    // This host polls state and does not drain SDL's input event queue. SDL documents manual
    // UpdateGamepads/UpdateJoysticks as the supported path when these events are disabled.
    impl_->prior_joystick_events = impl_->joystick_events();
    impl_->prior_gamepad_events = impl_->gamepad_events();
    impl_->set_gamepad_events(false);
    impl_->set_joystick_events(false);
    return true;
}

std::optional<SwitchControllerSample> HidSwitchController::poll(std::string& error) {
    error.clear();
    if (!impl_->initialized) {
        error = "native Switch controller backend is not initialized";
        return std::nullopt;
    }
    impl_->update();
    if (impl_->gamepad && !impl_->connected(impl_->gamepad)) {
        impl_->close(impl_->gamepad);
        impl_->gamepad = nullptr;
    }
    if (!impl_->gamepad) {
        int count = 0;
        SDL_JoystickID* ids = impl_->gamepads(&count);
        if (!ids) {
            error = impl_->error();
            if (error.empty()) error = "native controller enumeration failed";
            return std::nullopt;
        }
        for (int index = 0; ids && index < count; ++index) {
            if (impl_->vendor(ids[index]) == 0x057e && impl_->product(ids[index]) == 0x2069) {
                impl_->gamepad = impl_->open(ids[index]);
                if (!impl_->gamepad)
                    error = impl_->error();
                break;
            }
        }
        impl_->free(ids);
        if (!impl_->gamepad)
            return std::nullopt;
    }
    SwitchControllerSample sample{};
    sample.instance_id = impl_->id(impl_->gamepad);
    const auto properties = impl_->properties(impl_->gamepad);
    if (properties == 0) {
        error = impl_->error();
        return std::nullopt;
    }
    sample.rumble_supported =
        impl_->boolean_property(properties, SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false);
    for (unsigned index = 0; index < sample.state.axes.size(); ++index)
        sample.state.axes[index] = impl_->axis(impl_->gamepad, static_cast<SDL_GamepadAxis>(index));
    for (unsigned index = 0; index < sample.positional_buttons.size(); ++index)
        sample.positional_buttons[index] =
            impl_->button(impl_->gamepad, static_cast<SDL_GamepadButton>(index));
    // SDL3 reports positional face buttons. The pinned SDL2 Input profile requests labels.
    std::array<bool, 4> labels_seen{};
    for (int position = 0; position < 4; ++position) {
        const auto button = static_cast<SDL_GamepadButton>(position);
        const auto label = impl_->label(impl_->gamepad, button);
        unsigned destination = 4;
        switch (label) {
        case SDL_GAMEPAD_BUTTON_LABEL_A: destination = 0; break;
        case SDL_GAMEPAD_BUTTON_LABEL_B: destination = 1; break;
        case SDL_GAMEPAD_BUTTON_LABEL_X: destination = 2; break;
        case SDL_GAMEPAD_BUTTON_LABEL_Y: destination = 3; break;
        default: break;
        }
        if (destination == 4 || labels_seen[destination]) {
            error = "native Switch controller face labels are unresolved";
            return std::nullopt;
        }
        labels_seen[destination] = true;
        sample.state.buttons[destination] = sample.positional_buttons[position];
    }
    for (unsigned index = 4; index < sample.state.buttons.size(); ++index)
        sample.state.buttons[index] = sample.positional_buttons[index];
    return sample;
}

bool HidSwitchController::rumble(std::uint16_t low, std::uint16_t high,
                                std::uint32_t milliseconds, std::string& error) {
    error.clear();
    if (!impl_->gamepad || !impl_->connected(impl_->gamepad)) {
        error = "native Switch controller is disconnected";
        return false;
    }
    const auto properties = impl_->properties(impl_->gamepad);
    if (!impl_->boolean_property(properties, SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false)) {
        error = "native Switch controller does not support rumble";
        return false;
    }
    if (!impl_->rumble(impl_->gamepad, low, high, milliseconds)) {
        error = impl_->error();
        return false;
    }
    return true;
}
void HidSwitchController::shutdown() {
    if (impl_->gamepad) {
        impl_->rumble(impl_->gamepad, 0, 0, 0);
        impl_->close(impl_->gamepad);
        impl_->gamepad = nullptr;
    }
    if (impl_->initialized) {
        impl_->set_joystick_events(impl_->prior_joystick_events);
        impl_->set_gamepad_events(impl_->prior_gamepad_events);
        impl_->quit(SDL_INIT_GAMEPAD);
        impl_->initialized = false;
    }
    if (impl_->library) {
        dlclose(impl_->library);
        impl_->library = nullptr;
    }
}
} // namespace sadx::input::mac
