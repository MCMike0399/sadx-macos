#include "engine/view_check.hpp"

namespace sadx::view_check {
namespace {
// Each source fcomp/fcom boundary: culled only on an ordered less result;
// equality and unordered outcomes keep the point visible.
bool ordered_less(const fidelity::ExactFloat &left, float right) {
    return left.compare(right) == X87Comparison::less;
}
bool ordered_less(const fidelity::ExactFloat &left, const fidelity::ExactFloat &right) {
    return left.compare(right) == X87Comparison::less;
}
} // namespace

bool check_view(const NinjaVector &point, float radius, const State &state,
                const Services &services, fidelity::Context &arithmetic) {
    if (state.pending_logic_ticks != 0)
        return false;
    constexpr float kRadiusScale = 1.2f;
    const auto scaled = (fidelity::ExactFloat(arithmetic, radius) * kRadiusScale).rounded();
    const auto local = services.calculate_point(point);
    const auto exact_scaled = fidelity::ExactFloat(arithmetic, scaled);
    if (ordered_less(exact_scaled - fidelity::ExactFloat(arithmetic, local.z), state.draw_maximum))
        return false;
    const auto depth_near = -fidelity::ExactFloat(arithmetic, local.z) - exact_scaled;
    if (depth_near.compare(state.draw_minimum) == X87Comparison::greater)
        return false;
    // The source stores the perspective reciprocal -1 / depth_near and
    // multiplies every projected bound by it.
    const auto reciprocal = fidelity::ExactFloat(arithmetic, -1.0f) / depth_near;
    const auto scale_x = fidelity::ExactFloat(arithmetic, state.screen_distance) *
                         fidelity::ExactFloat(arithmetic, state.multi_x);
    const auto center_x = fidelity::ExactFloat(arithmetic, state.screen_center_x);
    const auto local_x = fidelity::ExactFloat(arithmetic, local.x);
    if (ordered_less(center_x + (local_x + exact_scaled) * scale_x * reciprocal, 0.0f))
        return false;
    constexpr float kScreenWidth = 640.0f;
    if (ordered_less(fidelity::ExactFloat(arithmetic, state.ratio_x) *
                         fidelity::ExactFloat(arithmetic, kScreenWidth),
                     center_x + (local_x - exact_scaled) * scale_x * reciprocal))
        return false;
    const auto scale_y = fidelity::ExactFloat(arithmetic, state.multi_y) *
                         fidelity::ExactFloat(arithmetic, state.screen_distance);
    const auto center_y = fidelity::ExactFloat(arithmetic, state.screen_center_y);
    const auto local_y = fidelity::ExactFloat(arithmetic, local.y);
    if (ordered_less(center_y - (local_y - exact_scaled) * scale_y * reciprocal, 0.0f))
        return false;
    if (ordered_less(fidelity::ExactFloat(arithmetic, state.ratio_y) *
                         fidelity::ExactFloat(arithmetic, kScreenWidth),
                     center_y - (local_y + exact_scaled) * scale_y * reciprocal))
        return false;
    return true;
}
} // namespace sadx::view_check
