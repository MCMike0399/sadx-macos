#pragma once

#include "cpu/x87_math.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>

namespace sadx {

struct PlayerInputTask {
    std::uint16_t flags;        // taskwk +0x04
    std::uint8_t player_index;  // counter byte +0x08
    std::uint8_t character_id;  // counter byte +0x09
    std::int32_t yaw;           // taskwk angle.y +0x18
    // Decoded eventwk.move.mode (+0x24 through taskwk.ewp +0x3c).
    // Missing is unknown/unavailable, never a false event state.
    std::optional<std::uint8_t> event_move_mode;
};

struct NormalizedPlayerInput {
    // A null-angle query does not consume direction. Preserve an unavailable
    // producer angle independently from an already-known magnitude.
    std::optional<std::int32_t> angle; // NormalizedAnalogs[player].direction
    float magnitude;   // NormalizedAnalogs[player].magnitude, source bits retained
};

// Source-bound globals, independently optional when their owning subsystem
// is unavailable. A query requires only the facts the selected path consumes.
struct PlayerInputRuntime {
    std::optional<std::int16_t> stage; // ssStageNumber at 0x3b22dcc
    std::optional<std::int16_t> act;   // low word at 0x3b22dec
    std::optional<std::uint16_t> current_character; // 0x4144e0 -> 0x3b22dc0
    std::array<std::optional<NormalizedPlayerInput>, 8> normalized;
    // playerwk.nocontimer (+0x0a), NOT playerwk.flag (+0x10).
    std::array<std::optional<std::uint16_t>, 8> no_control_timer;
    std::optional<std::int32_t> camera_yaw; // camera taskwk +0x18
};

enum class PlayerInputFailure {
    none,
    unavailable_event_move_mode,
    unavailable_stage,
    unavailable_act,
    unavailable_current_character,
    player_index_outside_known_slots,
    unavailable_normalized_input,
    unavailable_normalized_angle,
    unavailable_no_control_timer,
    missing_forced_angle_conversion,
    unavailable_forced_angle_conversion,
    unavailable_camera_yaw,
};

struct ForcedInputAngleResult {
    PlayerInputFailure failure;
    std::optional<std::int32_t> angle_units;
};

// FPATAN(-127,0), two binary64 multiplies and _ftol at 0x644340 remain an
// explicit arithmetic boundary. A provider must preserve selected x87 control
// and exceptions; no host atan2 or precomputed angle is substituted here.
using ForcedInputAngleConversion = std::function<ForcedInputAngleResult(X87Context&)>;

struct ForcedInputAngleConstants {
    double y;           // source 0x7e0050
    double x;           // source 0x7e0048
    double angle_units; // source 0x7dccd0
    double factor;      // source 0x7dcce8
};

// Actual arithmetic owner for the forced-stage branch. Constants and the
// x87 context remain explicit; the result is computed, not a fixed angle.
ForcedInputAngleResult convert_forced_input_angle(const ForcedInputAngleConstants&,
                                                 X87Context&);

struct PlayerInputResult {
    PlayerInputFailure failure = PlayerInputFailure::none;
    std::uint32_t source_boundary = 0;
    // A failed angle-output boundary can still retain known active=true.
    std::optional<bool> active;
    // Exact source output stores: bit0 angle, bit1 magnitude. Event-mode
    // active returns can leave both outputs untouched; activity alone cannot
    // establish that caller scratch values were initialized.
    std::uint8_t outputs_written = 0;
    explicit operator bool() const noexcept { return failure == PlayerInputFailure::none; }
};

// Recovered GetAnalog 0x43bdc0, a PGetSpeed 0x443f50 input-gate callee.
// Outputs are independently nullable. Inactive/event paths leave them intact.
// Failed dependencies preserve earlier x87 exceptions and source-order stores.
// FCOMP uses explicit software x87; this routine contains no SSE arithmetic.
PlayerInputResult get_player_input(const PlayerInputTask& task,
                                  const PlayerInputRuntime& runtime,
                                  X87Context& context,
                                  std::int32_t* angle,
                                  float* magnitude,
                                  const ForcedInputAngleConversion& forced_angle);

} // namespace sadx
