#include "engine/collision_component.hpp"
#include "math/ninja_length.hpp"

namespace sadx::collision_component {
namespace {
constexpr std::uint16_t kOwnsInfo = 1U << 15;
constexpr std::uint16_t kSpecialList = 1U << 6;
constexpr std::uint16_t kRetainedWorkFlags = 0b1111111111000000;
constexpr std::uint16_t kRetainedPrimaryFlags = 0b1111111111000011;
constexpr std::uint16_t kRetainedHitFlags = 0b1111111111111100;
constexpr std::uint8_t kRetainedDamage = 0b11101111;
constexpr std::uint32_t kPrimaryComponent = 56;
constexpr std::uint32_t kCleanupOwnership = 5;
constexpr std::uint32_t kCleanupInfo = 12;
constexpr std::uint32_t kWorkBytes = 168;
constexpr std::uint32_t kInfoBytes = 48;

// NOTSA: validates arena operations at the native memory boundary.
TaskWorkHandle require(TaskWorkResult result, Boundary boundary) {
    if (!result)
        throw Error(boundary, result.failure);
    return result.work;
}
// NOTSA: source memory operands remain distinct from retained arithmetic values.
fidelity::ExactFloat exact(Context &context, float value) {
    return fidelity::ExactFloat(context, value);
}
// NOTSA: inlined source form switch, retaining the scalar return until its caller's spill.
fidelity::ExactFloat extent(const Info &info, float center_length, Context &context) {
    switch (info.form) {
    case Form::Sphere:
        return exact(context, center_length) + info.a;
    case Form::Capsule:
        return (exact(context, center_length) + info.b) + info.a;
    default:
        break;
    }
    const auto squared = [&]() {
        if (info.form == Form::Cylinder || info.form == Form::CylinderAlternate)
            return exact(context, info.b) * info.b;
        const auto c_squared = exact(context, info.c) * info.c;
        const auto b_squared = exact(context, info.b) * info.b;
        return c_squared + b_squared;
    }();
    const auto a_squared = exact(context, info.a) * info.a;
    const float stored_squared = (squared + a_squared).rounded();
    return fidelity::ExactFloat::signed_square_root(context, stored_squared) + center_length;
}
} // namespace

std::shared_ptr<Work> component(TaskWorkArena &arena, const TaskWorkHandle &primary) {
    if (!primary)
        return {};
    const auto allocation =
        require(arena.reference(primary, kPrimaryComponent), Boundary::ComponentReference);
    if (!allocation)
        return {};
    if (!allocation->collision_state)
        throw Error(Boundary::ComponentState);
    return allocation->collision_state;
}

void free_work(TaskWorkArena &arena, const TaskWorkHandle &primary) {
    if (!primary)
        return;
    const auto allocation =
        require(arena.reference(primary, kPrimaryComponent), Boundary::ComponentReference);
    if (!allocation)
        return;
    std::uint8_t ownership = 0;
    require(arena.read(allocation, kCleanupOwnership, &ownership, sizeof(ownership)),
            Boundary::CleanupFlags);
    if (ownership & (1U << 7)) {
        const auto info =
            require(arena.reference(allocation, kCleanupInfo), Boundary::InfoReference);
        if (info) {
            require(arena.release(info), Boundary::InfoRelease);
            require(arena.set_reference(allocation, kCleanupInfo, {}), Boundary::InfoReference);
        }
    }
    require(arena.release(allocation), Boundary::WorkRelease);
    require(arena.set_reference(primary, kPrimaryComponent, {}), Boundary::PrimaryReference);
}

void free_collision(TaskWorkArena &arena, const TaskHandle &task) {
    if (task)
        free_work(arena, task->primary_work);
}

void clear_info(Work &work, std::uint16_t &status_flags) {
    work.flags &= kRetainedWorkFlags;
    work.hit_count = 0;
    status_flags &= kRetainedPrimaryFlags;
    work.own_number = -1;
    work.other_number = -1;
    work.other.reset();
    if (work.info_count > work.info.size())
        throw Error(Boundary::InfoCount);
    for (std::size_t i = 0; i < work.info_count; ++i)
        work.info[i].damage &= kRetainedDamage;
    for (auto &hit : work.hits) {
        hit.flags &= kRetainedHitFlags;
        hit.own_number = 255;
        hit.other_number = 255;
        hit.primary.reset();
    }
}

void calculate_range(Work *work, Context &context) {
    if (!work)
        return;
    if (work->info_count > work->info.size())
        throw Error(Boundary::InfoCount);
    float maximum = 0.0f;
    for (std::size_t i = 0; i < work->info_count; ++i) {
        const auto &info = work->info[i];
        const float length = vector_length(info.center, context).rounded();
        const auto radius = extent(info, length, context);
        if (radius.compare(maximum) == X87Comparison::greater)
            maximum = radius.rounded();
    }
    work->range = (exact(context, maximum) * 1.01f).rounded();
    if (exact(context, maximum).compare(0.1f) == X87Comparison::less)
        work->range = 25.0f;
}

InitResult initialize(TaskWorkArena &arena, const TaskHandle &task, std::uint16_t &status_flags,
                      const std::vector<Info> &source, std::int32_t count, std::uint8_t id,
                      Context &context) {
    if (!task)
        throw Error(Boundary::Task);
    if (!task->primary_work)
        return InitResult::NoPrimary;
    free_work(arena, task->primary_work);
    auto allocation = arena.allocate(kWorkBytes);
    if (!allocation)
        return InitResult::AllocationFailed;
    allocation.work->collision_state = std::make_shared<Work>();
    const auto bytes = static_cast<std::uint32_t>(count) * kInfoBytes;
    auto info_allocation = arena.allocate(bytes);
    if (info_allocation.failure == TaskWorkFailure::size_wrap)
        throw Error(Boundary::AllocationWrap, info_allocation.failure);
    if (!info_allocation) {
        // Original quirk: cleanup sees the old primary reference, still null.
        // The new work allocation is deliberately orphaned, not RAII-released.
        free_collision(arena, task);
        return InitResult::AllocationFailed;
    }
    if (bytes % kInfoBytes != 0)
        throw Error(Boundary::PartialInfoLayout);
    const auto copied_count = bytes / kInfoBytes;
    if (source.size() < copied_count)
        throw Error(Boundary::InfoCopy);
    auto &work = *allocation.work->collision_state;
    work.info.assign(source.begin(), source.begin() + copied_count);
    work.flags |= kOwnsInfo;
    const std::uint8_t cleanup_ownership = 1U << 7;
    require(arena.write(allocation.work, kCleanupOwnership, &cleanup_ownership, 1),
            Boundary::CleanupFlags);
    require(arena.set_reference(allocation.work, kCleanupInfo, info_allocation.work),
            Boundary::InfoReference);
    if (id == 2 || id == 3 || id == 7)
        work.flags |= kSpecialList;
    work.id = id;
    work.info_count = static_cast<std::uint16_t>(count);
    require(arena.set_reference(task->primary_work, kPrimaryComponent, allocation.work),
            Boundary::PrimaryReference);
    work.task = task;
    clear_info(work, status_flags);
    calculate_range(&work, context);
    return InitResult::Initialized;
}
} // namespace sadx::collision_component
