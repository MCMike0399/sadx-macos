#pragma once
#include <optional>
#include "engine/game_logic.hpp"
#include <array>
#include <cstdint>
#include <functional>
namespace sadx {
// The per-frame vibration update 785190 (registered by 785240), from exact
// instructions, over the four 0x18-stride entries at 3d08428: state +0 (0 idle,
// 1 start, 2 running, 3 stop; ResetLeafState's words, borrowed), strength byte
// +6, duration +0xc and counter +0x14. States 1/2 read the time (793da0, then
// 793e00 for the elapsed value) and switch to 3 once the duration is reached
// (unsigned); state 1 starts 791f80(i, min(|strength| * 0x594, 10000)) and
// counts; state 3 stops 792000(i) and returns to 0.
struct NinjaVibrationState {
    std::array<std::optional<std::uint8_t>,4> strength_at_3d0842e;   // stride 0x18
    std::array<std::optional<std::uint32_t>,4> duration_at_3d08434,counter_at_3d0843c;
};
struct NinjaVibrationCall {std::uint32_t source_call,callee;std::array<std::optional<std::uint32_t>,2> arguments={};};
struct NinjaVibrationRuntime {
    NinjaVibrationState& state;
    std::array<std::optional<std::uint32_t>,4>& state_at_3d08428;  // ResetLeafState
    // 793da0 (EAX unused), 793e00 (eax = the elapsed word it writes to the caller's local), 791f80, 792000.
    std::function<std::pair<GameLogicOwnerResult,std::optional<std::uint32_t>>(const NinjaVibrationCall&)> invoke;
};
enum class NinjaVibrationFailure {none,unknown_state,missing_owner,owner_failed};
struct NinjaVibrationResult {
    NinjaVibrationFailure failure=NinjaVibrationFailure::none;std::uint32_t source_boundary=0;std::optional<std::uint32_t> required_fact;
    explicit operator bool()const noexcept{return failure==NinjaVibrationFailure::none;}
};
NinjaVibrationResult run_vibration_785190(NinjaVibrationRuntime&);
} // namespace sadx
