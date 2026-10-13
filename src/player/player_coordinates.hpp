#pragma once

#include "player/player_motion.hpp"

#include <functional>
#include <optional>
#include <stdexcept>
#include <vector>

namespace sadx {

using NinjaMatrix = std::array<float, 16>;

enum class CoordinateFailure {
    none,
    invalid_matrix_stack,
    unknown_normalization_flag,
    missing_normal_angles,
    normal_angles_unavailable,
};

struct CoordinateResult {
    CoordinateFailure failure = CoordinateFailure::none;
    std::uint32_t source_boundary = 0;
    explicit operator bool() const noexcept { return failure == CoordinateFailure::none; }
};

class CoordinateError : public std::runtime_error {
public:
    explicit CoordinateError(CoordinateFailure failure);
    CoordinateFailure failure() const noexcept { return failure_; }
private:
    CoordinateFailure failure_;
};

// Native owner of source matrix slots, never a guest-pointer cast. Source depth
// is ONE-based: njInitMatrix sets depth1 while the current pointer is slot0.
// Popped slots retain bytes, as in the original fixed storage. Capacity is the
// caller-provided physical slot count; normalization flag is explicit/unknown.
class NinjaMatrixStack {
public:
    NinjaMatrixStack(std::vector<NinjaMatrix> slots, std::uint32_t source_depth,
                     std::optional<std::uint32_t> normalization_flag);
    bool push_ex(); // source capacity exhaustion is a no-op, not hidden growth
    bool push_matrix(const NinjaMatrix&); // 0x7850f0: supplied matrix copied only on success
    void pop_one(); // source target-depth-zero guard is a no-op
    void pop_ex(); // 0x7816e0: unlike pop_one, permits depth1 -> depth0
    NinjaMatrix& current();
    const NinjaMatrix& current() const;
    const std::vector<NinjaMatrix>& slots() const noexcept { return slots_; }
    std::uint32_t depth() const noexcept { return depth_; }
    std::optional<std::uint32_t> normalization_flag() const noexcept { return normalization_; }
private:
    std::vector<NinjaMatrix> slots_;
    std::uint32_t depth_;
    std::optional<std::uint32_t> normalization_;
};

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

// Owns source table/unit-matrix values, shares an explicit native matrix owner.
// Context is supplied at each operation; no ambient FPU profile or host trig.
class PlayerCoordinates {
public:
    PlayerCoordinates(NinjaMath::SineTable quarter_sine_table, NinjaMatrix unit_matrix,
                      NinjaMatrixStack& matrix_stack);
    // Transient numeric view over this owner's source table and selected FP
    // context; path sampling must not create a second persistent trig resource.
    NinjaMath source_math(X87Context&) const;

    // Source push-ex + unit-copy ownership. A capacity-exhausted push still
    // copies the unit into the current frame, exactly as callers do. No RAII
    // rollback hides partial source stores or exceptions.
    bool begin_unit_frame();
    void end_frame();
    // Collision callers use njPushMatrix(unit)/njPopMatrixEx. They have
    // different capacity/underflow semantics from the transform helpers above.
    bool push_unit_frame();
    bool push_current_frame(); // 0x7850f0 with explicit observed-null matrix argument
    void set_current_unit_matrix(); // source0x7875f0: no push or pop
    void end_ex_frame();
    NinjaMatrix& current_matrix();

    CoordinateResult world_to_player(const PlayerOrientation&, NinjaVector&, X87Context&);
    // A null task is an observed no-op in source0x43ec90.
    CoordinateResult task_to_world(const PlayerOrientation*, NinjaVector&, X87Context&);
    CoordinateResult floor_to_world(const NinjaVector& normal, NinjaVector&, std::int32_t yaw,
                                    const NormalAnglesProvider&, X87Context&);

    // Recovered primitives exposed for independent instruction comparisons.
    void rotate_x(NinjaMatrix&, std::int32_t angle, X87Context&);
    void rotate_y(NinjaMatrix&, std::int32_t angle, X87Context&);
    void rotate_z(NinjaMatrix&, std::int32_t angle, X87Context&);
    // 0x788df0: source alignment/conjugation with actual row stores. Axis
    // normalization is NOT invented: the second alignment uses raw axis.x.
    void rotate_axis(NinjaMatrix&,const NinjaVector& axis,std::int32_t angle,
                     const CoordinateAxisConstants&,X87Context&);
    // Actual matrix-owned projection portion 0x44b453..0x44b508. The caller
    // supplies the raw-stick conversion result and the retained pre-ftol ST0.
    FloorProjectionResult project_input_to_floor(const PlayerOrientation&,
        const NinjaVector& floor_normal,std::int32_t raw_stick_angle,
        std::int32_t camera_pitch,std::int32_t camera_yaw,X87Value retained_st0,
        const CoordinateAxisConstants&,X87Context&);
    // Actual0x781c10/0x781670, without invented vector normalization. Source
    // translation updates all four columns; point calculation adds translation.
    // Actual0x784c70 reloads each scalar for each row component store and
    // leaves translation row unchanged; explicit matrix owner resolves ECX0.
    void scale_matrix(NinjaMatrix&,float x,float y,float z,X87Context&);
    void translate_matrix(NinjaMatrix&, const NinjaVector&, X87Context&);
    void calculate_point(const NinjaMatrix&, const NinjaVector&, NinjaVector&, X87Context&);
    CoordinateResult calculate_vector(const NinjaMatrix&, const NinjaVector& input,
                                      NinjaVector& output, X87Context&);

private:
    NinjaMath::SineTable table_;
    NinjaMatrix unit_;
    NinjaMatrixStack& stack_;
};

} // namespace sadx
