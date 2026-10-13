#pragma once

#include "math/ninja_math.hpp"

#include <array>
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
    void reset_to_first() noexcept { depth_ = 1; } // 781250: current 3d0f818 = base 3d0fa1c, depth 3d0fa2c = 1
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

// The Ninja matrix primitives over one source matrix stack: push/pop/unit (0x7850f0, 0x7816e0,
// 0x7875f0), rotations, 0x788df0 axis rotation, 0x784c70 scale, 0x781c10 translate, 0x781670
// point and 0x784d00 vector calculation. Owns the source quarter-sine table and unit-matrix values;
// borrows the stack. Context is supplied at each operation; no ambient FPU profile or host trig.
// Engine code (sprites, projection) uses this directly; the player layer wraps it in PlayerCoordinates.
class NinjaMatrixPrimitives {
public:
    NinjaMatrixPrimitives(NinjaMath::SineTable quarter_sine_table, NinjaMatrix unit_matrix,
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

    // Recovered primitives exposed for independent instruction comparisons.
    void rotate_x(NinjaMatrix&, std::int32_t angle, X87Context&);
    void rotate_y(NinjaMatrix&, std::int32_t angle, X87Context&);
    void rotate_z(NinjaMatrix&, std::int32_t angle, X87Context&);
    // 0x788df0: source alignment/conjugation with actual row stores. Axis
    // normalization is NOT invented: the second alignment uses raw axis.x.
    void rotate_axis(NinjaMatrix&,const NinjaVector& axis,std::int32_t angle,
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

protected:
    NinjaMath::SineTable table_;
    NinjaMatrix unit_;
    NinjaMatrixStack& stack_;
};

// (0 - angle) & 0xffff, the source's negated Ninja angle.
std::int32_t ninja_wrapped_negation(std::int32_t angle);

} // namespace sadx
