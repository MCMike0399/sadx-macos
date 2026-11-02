#include "player/event_actions.hpp"

namespace sadx::event_actions {
namespace {
constexpr std::uint32_t kPrimaryEvent = 60;
constexpr std::uint32_t kEventBytes = 124, kNodeBytes = 24;
constexpr std::uint32_t kActionTimer = 60, kActionHead = 64, kActionOld = 68;
constexpr std::uint32_t kNodeNext = 16, kMoveMode = 36, kPathHead = 76;
constexpr std::uint16_t kInputPending = 1U << 12;

// NOTSA: arena failures are reported at the consumed operation, after prior stores.
TaskWorkHandle require(TaskWorkResult result, Boundary boundary) {
    if (!result)
        throw Error(boundary, result.failure);
    return result.work;
}
// NOTSA: only the two fixed CAlloc(1, size) calls reached by this cluster.
TaskWorkHandle allocate_zeroed(Context &context, std::uint32_t bytes, Boundary boundary) {
    const auto result = context.allocate ? context.allocate(bytes) : context.arena.allocate(bytes);
    if (result.failure == TaskWorkFailure::Exhausted)
        return {};
    const auto allocation = require(result, boundary);
    require(context.arena.fill(allocation, 0, 0, bytes), boundary);
    allocation->event_state = std::make_shared<Work>();
    return allocation;
}
// NOTSA: a null CAlloc result faults at the first node store, after link publication.
PlayerEventActionNode &node(const TaskWorkHandle &allocation, Boundary boundary) {
    if (!allocation || allocation->released())
        throw Error(boundary);
    if (!allocation->event_state || !allocation->event_state->node)
        throw Error(Boundary::EventState);
    return *allocation->event_state->node;
}
// NOTSA: native reference reads preserve null dereference boundaries.
TaskWorkHandle reference(TaskWorkArena &arena, const TaskWorkHandle &owner,
                         std::uint32_t field, Boundary boundary) {
    if (!owner || owner->released())
        throw Error(boundary);
    const auto result = arena.reference(owner, field);
    // A pointer load may borrow a released identity without dereferencing it.
    // ClearAction must still publish the null head before attempting old release.
    if (result.failure == TaskWorkFailure::ReleasedWork && result.work)
        return result.work;
    return require(result, boundary);
}
} // namespace

TaskWorkHandle event_work(TaskWorkArena &arena, const TaskWorkHandle &primary) {
    return reference(arena, primary, kPrimaryEvent, Boundary::EventReference);
}
PlayerEventActionNode *action_head(TaskWorkArena &arena, const TaskWorkHandle &primary) {
    const auto event = event_work(arena, primary);
    const auto head = reference(arena, event, kActionHead, Boundary::ActionHead);
    return head ? &node(head, Boundary::ActionHead) : nullptr;
}
bool live(const PlayerEventActionNode &value) noexcept {
    const auto allocation = value.allocation.lock();
    return allocation && !allocation->released();
}
PlayerEventActionNode *next_action(TaskWorkArena &arena, PlayerEventActionNode *value) {
    if (!value || !live(*value))
        throw Error(Boundary::ActionNext);
    const auto next = reference(arena, value->allocation.lock(), kNodeNext, Boundary::ActionNext);
    return next ? &node(next, Boundary::ActionNext) : nullptr;
}
std::uint8_t move_mode(TaskWorkArena &arena, const TaskWorkHandle &primary) {
    const auto event = event_work(arena, primary);
    std::uint8_t mode = 0;
    require(arena.read(event, kMoveMode, &mode, sizeof(mode)), Boundary::EventMode);
    return mode;
}
bool has_path(TaskWorkArena &arena, const TaskWorkHandle &primary) {
    const auto event = event_work(arena, primary);
    return reference(arena, event, kPathHead, Boundary::EventReference) != nullptr;
}
void initialize(Context &context, const TaskWorkHandle &primary) {
    const auto captured = primary;
    if (event_work(context.arena, captured))
        return;
    const auto event = allocate_zeroed(context, kEventBytes, Boundary::EventAllocation);
    require(context.arena.set_reference(captured, kPrimaryEvent, event), Boundary::EventReference);
    if (!event)
        throw Error(Boundary::EventMode);
    const std::uint8_t mode = 3;
    require(context.arena.write(event, 0, &mode, sizeof(mode)), Boundary::EventMode);
}
void append_action(Context &context, const TaskWorkHandle &primary, ActionHandle action,
                   texture_lifetime::List *textures, float speed, std::int32_t mode,
                   std::int32_t link_frames) {
    const auto event = event_work(context.arena, primary);
    auto owner = event;
    auto field = kActionHead;
    auto current = reference(context.arena, owner, field, Boundary::ActionHead);
    while (current) {
        owner = current;
        field = kNodeNext;
        current = reference(context.arena, owner, field, Boundary::ActionNext);
    }
    const auto allocation = allocate_zeroed(context, kNodeBytes, Boundary::NodeAllocation);
    if (allocation) {
        auto value = std::make_shared<PlayerEventActionNode>();
        value->allocation = allocation;
        allocation->event_state->node = value;
        context.state.node_identities.push_back(allocation);
    }
    // Original publishes even a null allocation before dereferencing the node.
    require(context.arena.set_reference(owner, field, allocation),
            field == kActionHead ? Boundary::ActionHead : Boundary::ActionNext);
    auto &value = node(allocation, Boundary::NodeMode);
    value.mode = static_cast<std::int8_t>(static_cast<std::uint8_t>(mode));
    value.link_frames = static_cast<std::int8_t>(static_cast<std::uint8_t>(link_frames));
    value.action.object = context.actions.object(action);
    value.action.motion = context.actions.motion(action);
    value.textures = textures;
    value.speed = speed;
}
void set_action(Context &context, const TaskHandle &task, ActionHandle action,
                texture_lifetime::List *textures, float speed, std::int32_t mode,
                std::int32_t link_frames) {
    if (task)
        append_action(context, task->primary_work, action, textures, speed, mode, link_frames);
}
void clear_actions(Context &context, const TaskWorkHandle &primary) {
    const auto event = event_work(context.arena, primary);
    auto current = reference(context.arena, event, kActionHead, Boundary::ActionHead);
    while (current) {
        const auto next = reference(context.arena, current, kNodeNext, Boundary::ActionNext);
        require(context.release ? context.release(current) : context.arena.release(current),
                Boundary::NodeRelease);
        current = next;
    }
    const auto old = reference(context.arena, event, kActionOld, Boundary::ActionOld);
    require(context.arena.set_reference(event, kActionHead, {}), Boundary::ActionHead);
    if (old) {
        require(context.release ? context.release(old) : context.arena.release(old), Boundary::OldRelease);
        require(context.arena.set_reference(event, kActionOld, {}), Boundary::ActionOld);
    }
    const std::uint32_t zero = 0;
    require(context.arena.write(event, kActionTimer, &zero, sizeof(zero)), Boundary::ActionTimer);
}
void clear_action(Context &context, const TaskHandle &task) {
    if (task)
        clear_actions(context, task->primary_work);
}
void set_input(const std::array<std::shared_ptr<PlayerTaskState>, 8> &registered_primary,
               std::uint8_t player, std::int8_t mode, const InputModeStore &store_mode) {
    // Slots outside the native registry are an unavailable source read, not clamped.
    if (player >= registered_primary.size())
        throw Error(Boundary::RegisteredPlayer);
    const auto primary = registered_primary[player];
    if (!primary)
        return;
    primary->orientation.flags |= kInputPending;
    if (!store_mode)
        throw Error(Boundary::InputMode);
    store_mode(*primary, static_cast<std::uint8_t>(mode));
}
} // namespace sadx::event_actions
