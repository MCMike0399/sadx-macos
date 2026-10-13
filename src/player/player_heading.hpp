#pragma once

#include "player/player_coordinates.hpp"

namespace sadx {

// Source 0x438350: mask both angles to 16 bits, sign-extend the wrapped
// difference and apply the caller's signed step with original NEG/ADD/SUB
// wrapping. Half-turn ties choose the negative direction.
std::int32_t adjust_angle(std::int32_t from,std::int32_t to,std::int32_t step) noexcept;
// Source 0x4383b0: unsigned size of the shortest wrapped 16-bit difference.
std::uint32_t angle_distance(std::int32_t first,std::int32_t second) noexcept;

struct PlayerHeadingResult {
    CoordinateResult coordinate;
    std::uint32_t source_boundary = 0;
    std::optional<NinjaVector> temporary_vector;
    explicit operator bool() const noexcept { return static_cast<bool>(coordinate); }
};

// Actual 0x443df0 owner. Convert the old local speed to world coordinates,
// update task yaw by at most the source 0x2000 step, then convert back before
// committing speed. Required matrix state comes from the selected owner.
// A failed callee retains its partial stores and any earlier task-yaw change.
PlayerHeadingResult turn_player_heading(PlayerOrientation&,PlayerMotion&,
                                        std::int32_t target_yaw,PlayerCoordinates&,X87Context&);

} // namespace sadx
