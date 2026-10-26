#pragma once

#include "cpu/fidelity_float.hpp"
#include "engine/task.hpp"
#include "math/ninja_math.hpp"
#include <array>
#include <stdexcept>

namespace sadx::collision_component {
using Context = fidelity::Context;

enum class Form : std::uint8_t { Sphere = 0, Cylinder = 1, CylinderAlternate = 2, Capsule = 6 };
struct Info {
    std::uint8_t kind = 0;
    Form form = Form::Sphere;
    std::uint8_t push = 0, damage = 0;
    std::uint32_t attributes = 0;
    NinjaVector center = {};
    float a = 0, b = 0, c = 0, d = 0;
    std::array<std::int32_t, 3> angles = {};
};
struct Hit {
    std::uint8_t own_number = 0, other_number = 0;
    std::uint16_t flags = 0;
    TaskWorkHandle primary;
};
struct Work {
    std::uint16_t id = 0;
    std::int16_t hit_count = 0;
    std::uint16_t flags = 0, info_count = 0;
    float range = 0;
    std::vector<Info> info;
    std::array<Hit, 16> hits = {};
    NinjaVector normal = {};
    std::weak_ptr<Task> task;
    std::int16_t own_number = 0, other_number = 0;
    std::weak_ptr<Work> other;
};
enum class Boundary {
    Task,
    ComponentReference,
    ComponentState,
    CleanupFlags,
    InfoReference,
    InfoAllocation,
    InfoCopy,
    PartialInfoLayout,
    AllocationWrap,
    WorkRelease,
    InfoRelease,
    PrimaryReference,
    InfoCount
};
class Error : public std::runtime_error {
  public:
    explicit Error(Boundary boundary, TaskWorkFailure failure = TaskWorkFailure::none)
        : std::runtime_error("collision component dependency is unavailable"), boundary(boundary),
          work_failure(failure) {}
    Boundary boundary;
    TaskWorkFailure work_failure;
};
enum class InitResult { Initialized, NoPrimary, AllocationFailed };

// NOTSA: borrows the component through the canonical primary allocation reference.
std::shared_ptr<Work> component(TaskWorkArena &, const TaskWorkHandle &primary);
// Source allocation identity and typed payload share the scheduler's existing arena.
// status_flags must alias the canonical primary owner, never its stale arena bytes.
// FUNCTION: SONIC 0x41caf0 CCL_Init
InitResult initialize(TaskWorkArena &, const TaskHandle &, std::uint16_t &status_flags,
                      const std::vector<Info> &, std::int32_t count, std::uint8_t id, Context &);
// FUNCTION: SONIC 0x41c4e0 FreeColliWork
void free_work(TaskWorkArena &, const TaskWorkHandle &primary);
// FUNCTION: SONIC 0x41caa0 Collision_Free
void free_collision(TaskWorkArena &, const TaskHandle &);
// FUNCTION: SONIC 0x418b60 CCL_ClearInfo
void clear_info(Work &, std::uint16_t &status_flags);
// FUNCTION: SONIC 0x41bac0 CCL_CalcRange
void calculate_range(Work *, Context &);
} // namespace sadx::collision_component
