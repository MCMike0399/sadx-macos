#pragma once

#include "engine/ninja_matrix.hpp"
#include "player/player_motion.hpp"

#include <functional>
#include <optional>

namespace sadx {

struct NormalAngles {
    std::int32_t inclination; // output1 of source0x4b8e80, rotation X
    std::int32_t roll;        // output2, rotation Z
};

struct FloorProjectionResult {
    CoordinateResult coordinate;
    std::uint32_t source_boundary = 0;
    std::optional<NinjaVector> camera_direction = std::nullopt;
    std::optional<NinjaVector> projected_direction = std::nullopt;
    std::optional<NinjaVector> forward_direction = std::nullopt;
    explicit operator bool() const noexcept { return static_cast<bool>(coordinate); }
};

struct NormalAnglesResult {
    CoordinateFailure failure;
    std::optional<NormalAngles> angles;
};

// Full normal→angles owner must preserve 0x4b8e80's source CRT asin/FPATAN and
// integer-conversion behavior. Empty/unknown provider cannot invent zero angles.
using NormalAnglesProvider = std::function<NormalAnglesResult(const NinjaVector&, X87Context&)>;

// The player's coordinate transforms (43ec90 family, 44b453 floor projection) over the engine's
// matrix primitives; PlayerCoordinates IS the shared matrix owner, so engine callers receive it as a
// NinjaMatrixPrimitives.
class PlayerCoordinates : public NinjaMatrixPrimitives {
public:
    using NinjaMatrixPrimitives::NinjaMatrixPrimitives;

    CoordinateResult world_to_player(const PlayerOrientation&, NinjaVector&, X87Context&);
    // A null task is an observed no-op in source0x43ec90.
    CoordinateResult task_to_world(const PlayerOrientation*, NinjaVector&, X87Context&);
    CoordinateResult floor_to_world(const NinjaVector& normal, NinjaVector&, std::int32_t yaw,
                                    const NormalAnglesProvider&, X87Context&);
    // Actual matrix-owned projection portion 0x44b453..0x44b508. The caller
    // supplies the raw-stick conversion result and the retained pre-ftol ST0.
    FloorProjectionResult project_input_to_floor(const PlayerOrientation&,
        const NinjaVector& floor_normal,std::int32_t raw_stick_angle,
        std::int32_t camera_pitch,std::int32_t camera_yaw,X87Value retained_st0,
        const CoordinateAxisConstants&,X87Context&);
};

} // namespace sadx
