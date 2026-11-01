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
    case TaskStep::pool_already_initialized:
        return 0x40b460;
    case TaskStep::pool_requires_empty_scheduler:
        return 0x40b466;
    case TaskStep::pool_allocation:
        return 0x40b46b;
    case TaskStep::pool_clear:
        return 0x40b4bd;
    case TaskStep::allocation_level:
        return 0x40b03e;
    case TaskStep::pool_required:
        return 0x40b030;
    case TaskStep::pool_exhaustion:
        return 0x40b037;
    case TaskStep::primary_initialization:
        return 0x40b884;
    case TaskStep::force_initialization:
        return 0x40b8ab;
    case TaskStep::motion_initialization:
        return 0x40b8ce;
    case TaskStep::auxiliary_initialization:
        return 0x40b8f1;
    case TaskStep::creation_rollback:
        return 0x40b927;
    case TaskStep::default_resource:
        return 0x40b918;
    case TaskStep::child_transform_read:
        return 0x40b98d;
    case TaskStep::child_transform_write:
        return 0x40b992;
    case TaskStep::membership:
        return 0x401000;
    case TaskStep::destruction_entry:
        return 0x40b586;
    case TaskStep::destruction_generation:
        return 0x40b596;
    case TaskStep::inactive_child_sibling:
        return 0x40b834;
    case TaskStep::inactive_child_parent:
        return 0x40b83e;
    case TaskStep::children_destroyed:
        return 0x40b5b4;
    case TaskStep::cleanup_resume:
        return 0x40b608;
    case TaskStep::collision_reference:
        return 0x40b617;
    case TaskStep::collision_flags:
        return 0x41c4f1;
    case TaskStep::collision_extra_reference:
        return 0x41c4f7;
    case TaskStep::collision_extra_release:
        return 0x41c4ff;
    case TaskStep::collision_extra_clear:
        return 0x41c507;
    case TaskStep::collision_release:
        return 0x41c50f;
    case TaskStep::collision_clear:
        return 0x41c517;
    case TaskStep::object_reference:
        return 0x40b628;
    case TaskStep::object_first_chain:
        return 0x417ea9;
    case TaskStep::object_first_next:
        return 0x417eb3;
    case TaskStep::object_first_release:
        return 0x417eb7;
    case TaskStep::object_first_clear:
        return 0x417ecb;
    case TaskStep::object_extra_reference:
        return 0x417ec6;
    case TaskStep::object_extra_release:
        return 0x417ed1;
    case TaskStep::object_extra_clear:
        return 0x417ed9;
    case TaskStep::object_counter_clear:
        return 0x417edc;
    case TaskStep::object_second_chain:
        return 0x418338;
    case TaskStep::object_second_next:
        return 0x418340;
    case TaskStep::object_second_release:
        return 0x418344;
    case TaskStep::object_second_clear:
        return 0x418356;
    case TaskStep::object_second_counter_clear:
        return 0x418360;
    case TaskStep::object_release:
        return 0x4187bc;
    case TaskStep::object_clear:
        return 0x4187c4;
    case TaskStep::primary_release:
        return 0x40b634;
    case TaskStep::force_release:
        return 0x40b647;
    case TaskStep::motion_release:
        return 0x40b65a;
    case TaskStep::auxiliary_release:
        return 0x40b66d;
    case TaskStep::child_dispatch_next:
        return 0x40b433;
    case TaskStep::indexed_dispatch_next:
        return 0x40b0e0;
    case TaskStep::execution_callback:
        return 0x40b10b;
    case TaskStep::none:
    case TaskStep::callback_dependency:
        return 0;
    }
    return 0;
}

inline std::uint32_t source_boundary(const TaskResult &result) {
    if (result.step == TaskStep::callback_dependency) {
        if (const auto diagnostic =
                std::dynamic_pointer_cast<const PlayerAuxiliaryEffectDiagnostic>(
                    result.callback_diagnostic))
            return diagnostic->result.source_boundary;
    }
    return source_boundary(result.step);
}

} // namespace sadx::task_reference
