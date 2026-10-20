#pragma once
#include "cpu/sse_math.hpp"
#include "input/input_initial_state.hpp"
#include "input/input_mod_pad.hpp"
#include "player/player_input_producer.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace sadx::native {
// NOTSA: source Windows virtual-key bindings, in KeyboardMouse's comparison order.
enum class KeyBinding : std::size_t {
    HalfPress,
    CenterCamera,
    A,
    B,
    X,
    Y,
    RightShoulder,
    LeftShoulder,
    Back,
    Start,
    LeftTrigger,
    RightTrigger,
    DpadUp,
    DpadDown,
    DpadLeft,
    DpadRight,
    LeftUp,
    LeftDown,
    LeftLeft,
    LeftRight,
    RightUp,
    RightDown,
    RightLeft,
    RightRight,
    Count
};
struct InputConfiguration {
    bool disable_mouse;
    std::uint8_t keyboard_player;
    std::array<std::uint32_t, static_cast<std::size_t>(KeyBinding::Count)> keys;
    std::array<InputModPadSettings, 8> pads;
};
// NOTSA: raw INI integers before the pinned DLL's narrowing conversions.
struct RawPadConfiguration {
    std::int32_t deadzone_left, deadzone_right;
    bool radial_left, radial_right;
    std::int32_t trigger_threshold;
};
struct RawInputConfiguration {
    bool disable_mouse;
    std::int32_t keyboard_player;
    std::array<std::int32_t, static_cast<std::size_t>(KeyBinding::Count)> keys;
    std::array<RawPadConfiguration, 8> pads;
};
InputConfiguration normalize_configuration(const RawInputConfiguration &);
// NOTSA: seven verified immutable source coefficients; no executable runtime reads.
PlayerInputProducerConstants controller_constants();
struct KeyboardKeyState {
    std::uint8_t held = 0, previous = 0, pressed = 0;
};
// NOTSA: borrowed game reset owners, initially zero in the source load image.
struct SoftResetState {
    std::uint8_t requested = 0, neighbor = 0, metal_sonic = 0;
};
std::uint32_t consume_soft_reset(SoftResetState &);
struct InputFrameContext {
    std::int32_t game_mode, demo_playing, demo_enabled, demo_cutscene;
};
struct AnalogSnapshot {
    std::int32_t angle;
    float magnitude;
};
struct PadSnapshot {
    input::ControllerInputValues buttons;
    bool connected;
    bool center_camera, half_press;
    Sse32 left_magnitude, right_magnitude;
};
// NOTSA: native producer for the pinned Input mod's keyboard and DreamPad path.
// The host owns event pumping/discovery. It supplies source virtual keys and raw
// SDL-domain samples; game mapping/deadzones are applied here exactly once.
// Mouse-enabled configurations fail explicitly. Demo playback stops at its unrecovered source
// boundary.
class Input {
  public:
    static std::unique_ptr<Input> create(const InputConfiguration &, SseContext &,
                                         std::string &error);
    ~Input();
    Input(const Input &) = delete;
    Input &operator=(const Input &) = delete;
    void key_event(std::uint32_t source_virtual_key, bool down);
    void focus_lost();
    void gamepad(std::size_t player, const std::optional<InputModGamepadState> &);
    // Raw producer only (qualification/oracles). Production uses poll_frame.
    bool poll(std::string &error);
    bool poll_frame(const InputFrameContext &, SoftResetState &, std::string &error);
    PadSnapshot snapshot(std::size_t player) const;
    std::array<std::uint8_t, 6> keyboard_keys() const;
    KeyboardKeyState key_state(std::size_t source_key_index) const;
    std::uint8_t check_key(std::size_t source_key_index) const;
    std::uint8_t switch_data() const;
    // Explicit raw KeyboardInput owner seam. Pinned Input never writes field0;
    // host modifier key events belong in key_event, not in this mask.
    void keyboard_modifiers(std::uint8_t source_modifier_bits);
    void enable_pad_reading();
    void disable_pad_reading();
    // GetControllers consumes the actual stage/camera owners, without advancing events.
    bool update_controllers(std::uint16_t stage, const PlayerInputCamera &,
                            const PlayerInputProducerConstants &, X87Context &, std::string &error);
    std::uint32_t held(std::size_t player) const;
    std::uint32_t pressed(std::size_t player) const;
    ControllerInputValues engine_snapshot(std::size_t player) const;
    std::uint32_t published_held(std::size_t player) const;
    AnalogSnapshot analog_snapshot(std::size_t player) const;
    std::array<std::uint32_t, 5> button_masks() const;

  private:
    struct Impl;
    explicit Input(std::unique_ptr<Impl>);
    std::unique_ptr<Impl> impl_;
};
} // namespace sadx::native
