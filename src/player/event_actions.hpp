#pragma once
#include "engine/character_pack.hpp"
#include "engine/task.hpp"
#include "engine/native_texture_lifetime.hpp"
#include "player/player_task.hpp"
#include <deque>
#include <stdexcept>

namespace sadx {
// EVENT_ACTION_LIST. The action fields are copied; the action container is not retained.
struct PlayerEventActionNode {
    std::int8_t mode = 0, link_frames = 0;
    character_pack::ActionResource action{};
    texture_lifetime::List *textures = nullptr;
    float speed = 0;
    // NOTSA: retain allocation identity without an ownership cycle or address reuse.
    std::weak_ptr<TaskWork> allocation;
};
}

namespace sadx::event_actions {
// eventwk: unrecovered look/move/path/face fields remain in the original allocation.
// List links have one owner: the arena's native references, also used by DestroyTask.
struct Work {
    std::shared_ptr<PlayerEventActionNode> node;
};
struct State {
    // NOTSA: stable identities for borrowed preview tails, including released nodes.
    // A released allocation never becomes live through arena-offset reuse.
    std::deque<TaskWorkHandle> node_identities;
};
enum class Boundary {
    Primary, EventReference, EventState, EventMode, EventAllocation,
    ActionHead, ActionNext, ActionOld, NodeAllocation, NodeMode, NodeLink,
    ActionObject, ActionMotion, NodeObject, NodeMotion, NodeTextures, NodeSpeed,
    NodeRelease, OldRelease, ActionTimer, RegisteredPlayer, RegisteredCounter, InputMode
};
class Error : public std::runtime_error {
  public:
    explicit Error(Boundary boundary, TaskWorkFailure failure = TaskWorkFailure::None)
        : std::runtime_error("event action dependency is unavailable"), boundary(boundary),
          work_failure(failure) {}
    Boundary boundary;
    TaskWorkFailure work_failure;
};
// NOTSA: separate source reads preserve action aliasing with the new node's stores.
struct ActionSource {
    virtual ~ActionSource() = default;
    virtual character_pack::ResourceId object(ActionHandle) = 0;
    virtual character_pack::ResourceId motion(ActionHandle) = 0;
};
struct Context {
    TaskWorkArena &arena;
    State &state;
    ActionSource &actions;
    // NOTSA: allocator adapters observe real allocation/release results and reentry.
    // Native composition leaves these empty and uses the scheduler arena directly.
    std::function<TaskWorkResult(std::uint32_t)> allocate = {};
    std::function<TaskWorkResult(const TaskWorkHandle &)> release = {};
};
// NOTSA: queries borrow the allocation held by the captured primary, not a registry slot.
TaskWorkHandle event_work(TaskWorkArena &, const TaskWorkHandle &primary);
PlayerEventActionNode *action_head(TaskWorkArena &, const TaskWorkHandle &primary);
PlayerEventActionNode *next_action(TaskWorkArena &, PlayerEventActionNode *);
bool live(const PlayerEventActionNode &) noexcept;
std::uint8_t move_mode(TaskWorkArena &, const TaskWorkHandle &primary);
bool has_path(TaskWorkArena &, const TaskWorkHandle &primary);
// FUNCTION: SONIC 0x417e80 FUN_00417e80
void initialize(Context &, const TaskWorkHandle &primary);
// FUNCTION: SONIC 0x417ef0 FUN_00417ef0
void append_action(Context &, const TaskWorkHandle &primary, ActionHandle,
                   texture_lifetime::List *, float speed, std::int32_t mode,
                   std::int32_t link_frames);
// FUNCTION: SONIC 0x42fe00 EV_SetAction
void set_action(Context &, const TaskHandle &, ActionHandle,
                texture_lifetime::List *, float speed, std::int32_t mode,
                std::int32_t link_frames);
// FUNCTION: SONIC 0x417ea0 FUN_00417ea0
void clear_actions(Context &, const TaskWorkHandle &primary);
// FUNCTION: SONIC 0x42fe40 EV_ClrAction
void clear_action(Context &, const TaskHandle &);
// NOTSA: binds the existing primary owner's secondary mode without another value.
using InputModeStore = std::function<void(PlayerTaskState &, std::uint8_t)>;
// FUNCTION: SONIC 0x441260 SetInputP
void set_input(const std::array<std::shared_ptr<PlayerTaskState>, 8> &registered_primary,
               std::uint8_t player, std::int8_t mode, const InputModeStore &store_mode);
} // namespace sadx::event_actions
