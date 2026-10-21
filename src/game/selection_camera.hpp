#pragma once
#include "engine/ninja_matrix.hpp"
#include "engine/ninja_view_native.hpp"

namespace sadx::selection_camera {
// NOTSA: borrows the actual camera task's primary work and existing view camera.
struct Pose {
    const std::array<std::int32_t, 3> &rotation;
    const NinjaVector &position;
};
struct Calls {
    virtual ~Calls() = default;
    // NOTSA: delegates to the existing view::set_camera source owner.
    virtual void set_camera(view::Camera &) = 0;
};
// FUNCTION: SONIC 0x433410 CharSelCamera
void update(Pose, view::Camera &, NinjaMatrixPrimitives &, Calls &, X87Context &);
} // namespace sadx::selection_camera
