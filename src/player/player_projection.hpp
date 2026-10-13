#pragma once
#include "ninja/ninja_math.hpp"
#include "cpu/crt_acos.hpp"

namespace sadx {
struct ProjectionAngleConstants {
    float negative_one; // binary32 source0x7dcb0c
    double first_factor; // binary64 source0x7dccd0
    double second_factor; // binary64 source0x7dccc8
};
enum class ProjectionAngleFailure { none, acos_unavailable };
struct ProjectionAngleResult {
    ProjectionAngleFailure failure = ProjectionAngleFailure::none;
    std::optional<std::int32_t> angle = std::nullopt;
    std::uint32_t source_boundary = 0;
    CrtAsinFailure crt_failure = CrtAsinFailure::none;
    std::uint32_t nested_source_boundary = 0;
    float stored_dot;
    std::optional<NinjaVector> cross = std::nullopt;
    std::optional<double> pending_stored_result = std::nullopt;
    std::optional<X87Value> pending_value = std::nullopt;
    explicit operator bool() const noexcept { return failure==ProjectionAngleFailure::none&&angle.has_value(); }
};
// Actual438410: retained-dot lower clamp, stored-dot upper clamp, real CRT
// acos/factors/ftol and optional source sign-direction dot. No input normalize.
ProjectionAngleResult projection_vector_angle(const NinjaVector& first,const NinjaVector& second,
    const NinjaVector* sign_direction,const CoordinateAxisConstants&,
    const ProjectionAngleConstants&,const CrtAsinRuntime&,X87Context&);
} // namespace sadx
