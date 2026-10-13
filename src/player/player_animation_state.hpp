#pragma once
#include "engine/task_work.hpp"
#include "engine/scene_resource_reference.hpp"
namespace sadx {
struct PlayerAnimationTable;
struct PlayerAnimationAllocatedMotion {
    TaskWorkHandle allocation;
    std::optional<TaskWorkHandle> data;
    std::optional<std::uint32_t> frame_count;
    std::optional<std::uint16_t> type,input_function;
};
struct PlayerAnimationAllocatedAction {
    TaskWorkHandle allocation;
    std::optional<SceneResourceReference> object;
    std::optional<std::shared_ptr<PlayerAnimationAllocatedMotion>> motion;
};
// Sole owner of the remaining mtnjvwk fields. Existing words120/122/124/126/128
// stay in PlayerPhysicsState and are borrowed at every call, never duplicated.
struct PlayerAnimationTail {
    std::optional<std::uint16_t> next_at_12a,timer_at_12c,flags_at_12e;
    std::optional<float> frame_at_130,start_at_134;
    std::optional<std::shared_ptr<const PlayerAnimationTable>> table_at_140;
    std::optional<std::shared_ptr<PlayerAnimationAllocatedAction>> buffer_at_148;
};
}
