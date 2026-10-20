#pragma once
#include <functional>
#include "engine/game_controller.hpp"
#include "input/input.hpp"
#include "input/input_mod_axes.hpp"
#include "cpu/sse_math.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>

namespace sadx {
// Input 2.6.1 (sadx-input-mod.dll SHA c5ccdcc7...) per-frame pad owner, from
// REA Ghidra Evidence in reference/input-dll-recovery: OnInput +0x6510
// (ev_5e46a6be...), poll_controllers +0x4790 (ev_a21b62bb...), DreamPad::poll
// +0x4080 (ev_92d691f1...), convert_axes +0x4400 (ev_c87c64b2...). The shipped
// DLL was built with EXTENDED_BUTTONS; it, not the upstream source, is authority.

// KeyboardMouse::dreamcast_data (DLL 1000e8ec..) plus its normalized magnitudes.
struct InputModKeyboardPad {
    std::uint32_t held=0;                 // +0x08
    std::uint16_t rtrigger=0,ltrigger=0;  // +0x18,+0x1a
    std::int16_t lx=0,ly=0,rx=0,ry=0;     // +0x1c..+0x22
    Sse32 normalized_l{0},normalized_r{0}; // 1000e8e8, 1000e744
};
// SDL_GameControllerGetAxis/GetButton values the DLL reads, in SDL2 enum order.
struct InputModGamepadState {
    std::array<std::int16_t,6> axes{};    // LEFTX, LEFTY, RIGHTX, RIGHTY, TRIGGERLEFT, TRIGGERRIGHT
    std::array<bool,15> buttons{};        // A,B,X,Y,BACK,GUIDE,START,LEFTSTICK,RIGHTSTICK,LSHOULDER,RSHOULDER,DPAD U/D/L/R
};
struct InputModPadSettings {
    bool allow_keyboard=false;            // +0x4c
    std::int16_t deadzone_l=0,deadzone_r=0; // +0x4e,+0x52
    bool radial_l=false,radial_r=false;   // +0x50,+0x54
    std::uint8_t trigger_threshold=0;     // +0x55
};
// One DLL DreamPad (0x60 bytes). The 52-byte dc_pad is the shared record type.
struct InputModDreamPad {
    GameLogicController record;           // +0x00..+0x33
    bool connected=false;                 // +0x3c
    bool e_held=false,half_press=false;   // +0x3d,+0x3e (persist between polls)
    std::optional<Sse32> normalized_l,normalized_r; // +0x44,+0x48
    InputModPadSettings settings;
    // Rumble state written by the mod's RumbleA_r/RumbleB_r (not ported): flags +0x3f, the dword at +0x40.
    std::uint8_t rumble_flags=0;std::uint32_t rumble_word=0;
};
// The mod's Jumps replacement for pdVibMxStop 785330 (DLL +0x8d80, REA Ghidra ev_76cfd0e6...):
// free each nonzero rumble task in the eight pairs at 1000ec60 through the game's
// 4b4160 and clear it; for each pad with nonzero rumble flags clear the dword at
// +0x40, clear flag bits 0-1 and SDL_GameControllerRumble(pad, 0, 0, 0). EAX = 0.
// It does not touch the game's 3d08428 vibration states (the vanilla 785330 does).
struct InputModRumbleTasks {std::array<std::uint32_t,16> tasks_at_1000ec60{};};
struct InputModVibrationStopOwners {
    std::function<bool(std::uint32_t task)> free_task;   // the game's 4b4160 (an owner)
    std::function<bool(std::size_t pad)> stop_rumble;     // SDL_GameControllerRumble(pad +0x34, 0, 0, 0)
};
bool input_mod_vibration_stop(InputModRumbleTasks&,std::array<InputModDreamPad,8>&,const InputModVibrationStopOwners&);
// DLL .rdata constants used by DreamPad::poll beyond convert_axes's own set.
struct InputModPadConstants {
    InputModAxesConstants axes;  // 1000bb6c,1000bbb0,1000bba0,1000bba4,1000bb7c
    Sse32 trigger_scale;          // 1000bba8 (255.0f)
    Sse32 half_press_scale;       // 1000bb78 (0.5f)
};
// poll_controllers connect handling: the opened pad's Support word becomes 0x3f0fff.
void input_mod_connect(InputModDreamPad&);
enum class InputModFailure {none,unknown_old_buttons,unknown_raw_controller,null_raw_controller,demo_unported,unknown_fact,fp_failed,axes_incomplete};
struct InputModResult {
    InputModFailure failure=InputModFailure::none;
    std::uint32_t source_boundary=0; // DLL VA
    bool polled=false,soft_reset=false;
    explicit operator bool()const noexcept{return failure==InputModFailure::none;}
};
// +0x4080, using the verified convert_input_mod_261_axes for +0x4400. The
// gamepad state is read only when the pad is connected.
InputModResult input_mod_poll(InputModDreamPad&,const InputModKeyboardPad&,const InputModGamepadState&,bool demo_at_1000e708,const InputModPadConstants&,SseContext&);
// poll_controllers tail: each pad's 52 bytes go to the DLL raw_input[i]
// (1000e748, all eight) and pads 0..3 also to the fixed engine ControllersRaw
// records at 3b0e9c8. Unknown bytes copy as unknown; a missing record
// allocation is a required boundary.
using InputModRawRecords=std::array<std::shared_ptr<GameLogicController>,8>;
InputModResult input_mod_publish(const std::array<InputModDreamPad,8>&,
    const std::array<std::shared_ptr<GameLogicController>,4>& controllers_raw_at_3b0e9c8,
    const InputModRawRecords& raw_input_at_1000e748);
// InitRawControllers hook (40f451 -> +0x4be0 -> +0x4b80, exact bytes): after
// vanilla 40f3a0 points ControllerPointers at ControllersRaw, the Input mod
// repoints all eight at its own raw_input records. The engine therefore reads
// raw_input; GameLogicControllers models entries 0..3 (no native consumer of
// 4..7 exists yet).
void input_mod_init_raw_controllers(const InputModRawRecords&,GameLogicControllers& controller_pointers_at_3b0e77c);
// EnableController/DisableController replacements (40ef70 -> +0x4bf0,
// 40efa0 -> +0x4c30; REA ev_f29f92cf..., ev_669e4f4c...). They maintain the
// mod's own 8-byte table (1000e73c), which the mod's Init also makes the source
// read: its WriteData repoints the 909fb4/909fb5 operands at 40fe0d/40fe2f (in
// 40fdc0), 40ef80..40efd8 and 40f30c to it. Index >= 2 also sets/clears entries 0 and 1 (one word store);
// index > 8 recurses over 0..7. Enable(8) writes one byte past the table into
// the low byte of the keyboard normalized_r float (1000e744); Disable(8) hits
// the invalid-parameter abort.
struct InputModControllerTable {std::array<std::uint8_t,8> enabled_at_1000e73c{};};
enum class InputModTableFailure {none,overflow_target_missing,invalid_parameter_abort};
InputModTableFailure input_mod_enable_controller(std::uint8_t index,InputModControllerTable&,InputModKeyboardPad* overflow_target);
InputModTableFailure input_mod_disable_controller(std::uint8_t index,InputModControllerTable&);
// OnInput after poll_controllers: soft reset (held 0x606 and released Start,
// GameMode not 1/8). The demo-playback branch is not ported and stops.
struct InputModFrameState {
    // Read-only game globals, borrowed from their owners; null is an unknown fact.
    const std::optional<std::int32_t>* game_mode_at_3abdc7c=nullptr;
    const std::optional<std::int32_t>* demo_playing_at_3b2a2e4=nullptr;
    std::optional<std::int32_t> demo_cutscene_at_3b2a2e8;
    std::optional<std::uint32_t> demo_enabled; // *DAT_1000e060
    bool demo_at_1000e708=false;
    std::optional<std::uint8_t> soft_reset_byte; // *DAT_1000e05c
};
InputModResult input_mod_on_input_tail(const GameLogicControllers&,InputModFrameState&);
// KeyboardMouse::poll output in the Input-mod pad layout, from a keyboard-only
// InputState tick (its ramp/normalize already follow the DLL goldens). L/R
// pressure is 255 when the button is held, as in KeyboardMouse::poll.
InputModKeyboardPad input_mod_keyboard_pad(const input::TickInput& keyboard_only);
// Updated-profile 40f460: the Input mod patches 40f460 to its final RET at
// 40fdb3, which the loader (eb4cdcca, built-in SDL off) replaces with OnInput
// events: Input 2.6.1 OnInput (poll_controllers + tail), then Speedrun 2.0's
// empty OnInput (REA ev_3af47a88...).
struct InputModFrameOwners {
    std::array<InputModDreamPad,8>& pads;
    const std::array<std::shared_ptr<GameLogicController>,4>& controllers_raw_at_3b0e9c8;
    const InputModRawRecords& raw_input_at_1000e748;
    const GameLogicControllers& controller_pointers_at_3b0e77c;
};
InputModResult input_mod_on_input(InputModFrameOwners&,const InputModKeyboardPad&,const std::array<InputModGamepadState,8>&,InputModFrameState&,const InputModPadConstants&,SseContext&);
// NOTSA: complete native frame facts, adapted only at the existing Input owner.
struct NativeInputModFrame {
    std::int32_t game_mode, demo_playing, demo_enabled, demo_cutscene;
    bool demo_active;
    std::uint8_t soft_reset;
};
struct NativeInputModOwners {
    std::array<InputModDreamPad,8>& pads;
    const std::array<std::shared_ptr<GameLogicController>,4>& engine_raw;
    const InputModRawRecords& raw_input;
};
InputModResult input_mod_on_input(NativeInputModOwners&, const InputModKeyboardPad&,
    const std::array<InputModGamepadState,8>&, NativeInputModFrame&,
    const InputModPadConstants&, SseContext&);
} // namespace sadx
