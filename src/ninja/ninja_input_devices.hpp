#pragma once
#include <optional>
#include "game/game_logic.hpp"
#include <cstdint>
#include <functional>
namespace sadx {
// The per-frame DirectInput device polls the 77f110 and 77efc0 callbacks jump to:
// 792100 (joysticks; returns when the count at 3d08814 is 0) and 791df0 (mouse;
// returns when the device at 3d08834 is null). Under the pinned Input mod the
// device setups (7928e0, 791d60) are NOPs, so both return at once; the device
// bodies (first instructions 792122 and 791e0b) are owners.
struct NinjaInputDeviceState {std::optional<std::uint32_t> joysticks_at_3d08814,mouse_at_3d08834;};
enum class NinjaInputDeviceFailure {none,unknown_state,missing_owner,owner_failed};
struct NinjaInputDeviceResult {
    NinjaInputDeviceFailure failure=NinjaInputDeviceFailure::none;std::uint32_t source_boundary=0;std::optional<std::uint32_t> required_fact;
    explicit operator bool()const noexcept{return failure==NinjaInputDeviceFailure::none;}
};
using NinjaInputDeviceBody=std::function<GameLogicOwnerResult(std::uint32_t body)>;
NinjaInputDeviceResult run_joystick_poll_792100(const NinjaInputDeviceState&,const NinjaInputDeviceBody&);
NinjaInputDeviceResult run_mouse_poll_791df0(const NinjaInputDeviceState&,const NinjaInputDeviceBody&);
} // namespace sadx
