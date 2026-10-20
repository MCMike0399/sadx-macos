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

std::optional<HleXInputGamepad> xinput_sample(const SwitchControllerSample& sample) noexcept {
    const auto& state = sample.state;
    // Switch2Pro triggers are digital. Reject an unexpected intermediate raw trigger instead of
    // silently quantizing a future device's analog input through the synthetic 8-bit transport.
    if ((state.axes[4] != 0 && state.axes[4] != 32767) ||
        (state.axes[5] != 0 && state.axes[5] != 32767))
        return std::nullopt;
    HleXInputGamepad out{};
    constexpr unsigned masks[15] = {4096, 8192, 16384, 32768, 32, 1024, 16,
                                    64, 128, 256, 512, 1, 2, 4, 8};
    for (unsigned index = 0; index < state.buttons.size(); ++index)
        if (state.buttons[index])
            out.buttons |= static_cast<std::uint16_t>(masks[index]);
    out.left_x = state.axes[0];
    out.right_x = state.axes[2];
    // The reference raw-input HID parser and pinned SDL2 produce ~XInputY, not -XInputY.
    // This exact inverse includes both signed16 endpoints and neutral zero.
    out.left_y = static_cast<std::int16_t>(-1 - static_cast<int>(state.axes[1]));
    out.right_y = static_cast<std::int16_t>(-1 - static_cast<int>(state.axes[3]));
    out.left_trigger = state.axes[4] == 32767 ? 255 : 0;
    out.right_trigger = state.axes[5] == 32767 ? 255 : 0;
    return out;
}

struct HostControllerInput::Impl {
    HidSwitchController native;
    GCController* apple = nil;
    bool native_ready = false;
    bool native_selected = false;
    bool motors = false;
    std::uint32_t native_instance = 0;
    std::uint64_t generation = 0;
};
HostControllerInput::HostControllerInput() : impl_(std::make_unique<Impl>()) {}
HostControllerInput::~HostControllerInput() { shutdown(); }
bool HostControllerInput::initialize(const std::string& native_library, std::string& error) {
    if (impl_->native_ready) {
        error = "native controller input already initialized";
        return false;
    }
    impl_->native_ready = impl_->native.initialize(native_library, error);
    return impl_->native_ready;
}
std::optional<HleXInputGamepad> HostControllerInput::poll(std::string& error) {
    error.clear();
    // Preserve the app's existing choice and GameController quantization for supported devices.
    for (GCController* controller in [GCController controllers]) {
        if (auto sample = xinput_sample(controller)) {
            if (impl_->native_selected) {
                std::string stopped;
                impl_->native.rumble(0, 0, 0, stopped);
            }
            if (impl_->apple != controller || impl_->native_selected)
                ++impl_->generation;
            impl_->apple = controller;
            impl_->native_selected = false;
            impl_->native_instance = 0;
            impl_->motors = false; // GameController haptics are not modeled by this adapter.
            return sample;
        }
    }
    if (!impl_->native_ready) {
        error = "native Switch controller backend is unavailable";
        return std::nullopt;
    }
    auto sample = impl_->native.poll(error);
    if (!sample) {
        impl_->apple = nil;
        impl_->native_selected = false;
        impl_->native_instance = 0;
        impl_->motors = false;
        return std::nullopt;
    }
    const auto converted = xinput_sample(*sample);
    if (!converted) {
        error = "native Switch controller supplied unsupported analog trigger values";
        impl_->motors = false;
        return std::nullopt;
    }
    if (!impl_->native_selected || sample->instance_id != impl_->native_instance)
        ++impl_->generation;
    impl_->apple = nil;
    impl_->native_selected = true;
    impl_->native_instance = sample->instance_id;
    impl_->motors = sample->rumble_supported;
    return converted;
}
bool HostControllerInput::rumble_supported() const noexcept { return impl_->motors; }
std::uint64_t HostControllerInput::connection_id() const noexcept { return impl_->generation; }
bool HostControllerInput::rumble(std::uint16_t low, std::uint16_t high, std::string& error) {
    if (!impl_->native_selected || !impl_->motors) {
        error = "selected controller has no native vibration owner";
        return false;
    }
    // XInput owns duration through successive SetState requests. SDL UINT32_MAX remains active
    // until the next request/stop, rather than inventing a pulse duration in the host adapter.
    return impl_->native.rumble(low, high, UINT32_MAX, error);
}
void HostControllerInput::shutdown() {
    impl_->native.shutdown();
    impl_->native_ready = false;
    impl_->native_selected = false;
    impl_->native_instance = 0;
    impl_->apple = nil;
    impl_->motors = false;
}
}
