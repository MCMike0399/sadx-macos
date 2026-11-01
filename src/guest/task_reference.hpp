#pragma once

#include "engine/task.hpp"
#include "player/player_auxiliary_effect.hpp"

namespace sadx::task_reference {

inline TaskWorkArena::PointerEncoder pointer_encoder(std::optional<std::uint32_t> base) {
    if (!base)
        return {};
    return [base](std::uint32_t offset) { return std::optional<std::uint32_t>{*base + offset}; };
}

inline void unavailable_default(TaskScheduler &scheduler) {
    scheduler.set_default_resource_reader(
        []() -> std::shared_ptr<void> { throw TaskResourceUnavailable{}; });
}

inline std::uint32_t source_boundary(TaskStep step) {
    switch (step) {
    case TaskStep::PoolAlreadyInitialized:
        return 0x40b460;
    case TaskStep::PoolRequiresEmptyScheduler:
        return 0x40b466;
    case TaskStep::PoolAllocation:
        return 0x40b46b;
    case TaskStep::PoolClear:
        return 0x40b4bd;
    case TaskStep::AllocationLevel:
        return 0x40b03e;
    case TaskStep::PoolRequired:
        return 0x40b030;
    case TaskStep::PoolExhaustion:
        return 0x40b037;
    case TaskStep::PrimaryInitialization:
        return 0x40b884;
    case TaskStep::ForceInitialization:
        return 0x40b8ab;
    case TaskStep::MotionInitialization:
        return 0x40b8ce;
    case TaskStep::AuxiliaryInitialization:
        return 0x40b8f1;
    case TaskStep::CreationRollback:
        return 0x40b927;
    case TaskStep::DefaultResource:
        return 0x40b918;
    case TaskStep::ChildTransformRead:
        return 0x40b98d;
    case TaskStep::ChildTransformWrite:
        return 0x40b992;
    case TaskStep::Membership:
        return 0x401000;
    case TaskStep::DestructionEntry:
        return 0x40b586;
    case TaskStep::DestructionGeneration:
        return 0x40b596;
    case TaskStep::InactiveChildSibling:
        return 0x40b834;
    case TaskStep::InactiveChildParent:
        return 0x40b83e;
    case TaskStep::ChildrenDestroyed:
        return 0x40b5b4;
    case TaskStep::CleanupResume:
        return 0x40b608;
    case TaskStep::CollisionReference:
        return 0x40b617;
    case TaskStep::CollisionFlags:
        return 0x41c4f1;
    case TaskStep::CollisionExtraReference:
        return 0x41c4f7;
    case TaskStep::CollisionExtraRelease:
        return 0x41c4ff;
    case TaskStep::CollisionExtraClear:
        return 0x41c507;
    case TaskStep::CollisionRelease:
        return 0x41c50f;
    case TaskStep::CollisionClear:
        return 0x41c517;
    case TaskStep::ObjectReference:
        return 0x40b628;
    case TaskStep::ObjectFirstChain:
        return 0x417ea9;
    case TaskStep::ObjectFirstNext:
        return 0x417eb3;
    case TaskStep::ObjectFirstRelease:
        return 0x417eb7;
    case TaskStep::ObjectFirstClear:
        return 0x417ecb;
    case TaskStep::ObjectExtraReference:
        return 0x417ec6;
    case TaskStep::ObjectExtraRelease:
        return 0x417ed1;
    case TaskStep::ObjectExtraClear:
        return 0x417ed9;
    case TaskStep::ObjectCounterClear:
        return 0x417edc;
    case TaskStep::ObjectSecondChain:
        return 0x418338;
    case TaskStep::ObjectSecondNext:
        return 0x418340;
    case TaskStep::ObjectSecondRelease:
        return 0x418344;
    case TaskStep::ObjectSecondClear:
        return 0x418356;
    case TaskStep::ObjectSecondCounterClear:
        return 0x418360;
    case TaskStep::ObjectRelease:
        return 0x4187bc;
    case TaskStep::ObjectClear:
        return 0x4187c4;
    case TaskStep::PrimaryRelease:
        return 0x40b634;
    case TaskStep::ForceRelease:
        return 0x40b647;
    case TaskStep::MotionRelease:
        return 0x40b65a;
    case TaskStep::AuxiliaryRelease:
        return 0x40b66d;
    case TaskStep::ChildDispatchNext:
        return 0x40b433;
    case TaskStep::IndexedDispatchNext:
        return 0x40b0e0;
    case TaskStep::ExecutionCallback:
        return 0x40b10b;
    case TaskStep::None:
    case TaskStep::CallbackDependency:
        return 0;
    }
    return 0;
}

inline std::uint32_t source_boundary(const TaskResult &result) {
    if (result.step == TaskStep::CallbackDependency) {
        if (const auto diagnostic =
                std::dynamic_pointer_cast<const PlayerAuxiliaryEffectDiagnostic>(
                    result.callback_diagnostic))
            return diagnostic->result.source_boundary;
    }
    return source_boundary(result.step);
}

} // namespace sadx::task_reference
