#pragma once

#include "player/player_coordinates.hpp"
#include "player/player_input.hpp"

namespace sadx {
enum class PlayerRotationFailure { none, unavailable_timer, unavailable_gravity,
    coordinate_unavailable, input_unavailable };
struct PlayerRotationResult {
    PlayerRotationFailure failure = PlayerRotationFailure::none;
    std::uint32_t source_boundary = 0;
    CoordinateFailure coordinate_failure = CoordinateFailure::none;
    PlayerInputFailure input_failure = PlayerInputFailure::none;
    std::optional<NinjaVector> temporary_vector = std::nullopt;
    std::uint32_t nested_source_boundary = 0;
    explicit operator bool() const noexcept { return failure==PlayerRotationFailure::none; }
};

} // namespace sadx
