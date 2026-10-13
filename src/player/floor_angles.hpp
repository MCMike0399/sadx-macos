#pragma once

#include "cpu/crt_asin.hpp"
#include "player/player_coordinates.hpp"
#include <optional>

namespace sadx {
struct FloorAngleConstants {
    float upper_cut;
    float lower_cut;
    float axis_epsilon;
    double angle_units;
    double positive_factor;
    double negative_factor;
};
struct FloorAnglesResult {
    CrtAsinFailure failure;
    std::optional<NormalAngles> angles;
    std::uint32_t source_boundary;
};

// The source writes its first angle after FPATAN, before roll scaling/FISTP.
// Preserve that partial fact if later native arithmetic stops. This retains
// X87Error catch compatibility and does not invent a second angle value or
// claim equivalence with hardware's deferred exception delivery.
class FloorAngleArithmeticError : public X87Error {
public:
    FloorAngleArithmeticError(const X87Error&, std::int32_t stored_inclination,
                              std::uint32_t source_boundary);
    std::int32_t stored_inclination() const noexcept { return inclination_; }
    std::uint32_t source_boundary() const noexcept { return boundary_; }
private:
    std::int32_t inclination_;
    std::uint32_t boundary_;
};

// Source 0x4b8e80: no normalization/clamping of the supplied normal. Source
// constants and CRT runtime state are caller-selected, not implicit defaults.
FloorAnglesResult floor_normal_angles(const NinjaVector&, const FloorAngleConstants&,
                                      const CrtAsinRuntime&, X87Context&);
} // namespace sadx
