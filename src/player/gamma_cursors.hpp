#pragma once

#include "engine/task_work.hpp"
#include <array>
#include <stdexcept>

namespace sadx::gamma_cursors {
// NOTSA: bounded native pointer into shared raw words. An interior pointer may
// index preceding elements; imported templates use origin zero.
struct WordView {
    const std::vector<std::uint32_t> *storage = nullptr;
    std::int32_t origin = 0;
};
// WIREMODEL inputs borrow canonical imported arrays. Components remain raw words:
// initialization copies them without performing floating-point operations.
struct WireModel {
    WordView points, colors;
    const std::vector<std::int16_t> *indices = nullptr;
    std::int16_t line_count = 0;
    std::uint16_t padding = 0;
    TaskWorkHandle expanded_points;
    TaskWorkHandle expanded_colors;
    const void *texture = nullptr;
    std::int32_t point_count = 0;
};
struct Templates {
    // GLOBAL: SONIC 0x981ea0 GammaCursorTemplate1 size=32
    WireModel first;
    // GLOBAL: SONIC 0x981ec0 GammaCursorTemplate2 size=32
    WireModel second;
};
enum class Boundary { IndexRead, PointRead, PointWrite, ColorRead, ColorWrite };
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, std::int32_t vertex, unsigned component,
          TaskWorkFailure failure = TaskWorkFailure::none)
        : std::runtime_error("Gamma cursor memory boundary"), boundary(boundary),
          vertex(vertex), component(component), work_failure(failure) {}
    Boundary boundary;
    std::int32_t vertex;
    unsigned component;
    TaskWorkFailure work_failure;
};
// FUNCTION: SONIC 0x4c2d40 GammaExpandCursor
void expand(WireModel &, TaskWorkArena &);
// NOTSA: native implementation of the existing GammaInitializeCursors seam.
// Template two must be read only after expansion of the first destination returns.
void initialize(WireModel &first, WireModel &second, const Templates &, TaskWorkArena &);
} // namespace sadx::gamma_cursors
