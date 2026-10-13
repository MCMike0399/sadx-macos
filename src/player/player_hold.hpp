#pragma once
#include "player/player_position.hpp"
#include "engine/task.hpp"

namespace sadx {
enum class PlayerHoldFailure {
    none, missing_player_lookup, unknown_player_reference,
    null_required_player_work, unknown_held_task, missing_work_arena,
    work_read_failed, unknown_collision_reference, null_required_collision,
    unresolved_collision, unknown_collision_flags, unknown_hit_reference,
    unknown_owning_task, null_required_owning_task, unresolved_owning_task,
    unknown_actor_reference, null_required_actor, unresolved_actor,
    unknown_actor_flags, unknown_actor_id
};
struct PlayerHoldResult {
    PlayerHoldFailure failure=PlayerHoldFailure::none;
    std::uint32_t source_boundary=0;
    TaskWorkFailure work_failure=TaskWorkFailure::none;
    std::optional<std::uint32_t> flags;
    explicit operator bool()const noexcept{return failure==PlayerHoldFailure::none;}
};
struct PlayerHoldRuntime {
    // Source3b3cdf0 indexed by the full player-index byte. No source bounds
    // check exists here. Missing entry is unknown; present nullptr is known
    // null, which the source dereferences rather than returning zero.
    std::function<std::optional<const PlayerPhysicsState*>(std::uint8_t)> player_lookup;
    // Actual native task work owner. Never decode source pointers into host
    // addresses or treat unknown bytes as allocator zeros.
    std::shared_ptr<const TaskWorkArena> work_arena;
};

// Actual43b9c0 is a read-only integer flag query. It neither calls exec,
// moves a held object nor touches FPU state. Required facts are consumed in
// instruction order; only observed source null/flag branches return zero.
PlayerHoldResult query_player_hold(const PlayerTaskState&,
    const PlayerPositionCollisionScene&,const PlayerHoldRuntime&);
// Borrows the caller's live runtime; temporary runtimes are rejected so
// changing registration/arena facts cannot leave a stale binding snapshot.
decltype(PlayerPositionCallees::hold) bind_player_hold(const PlayerHoldRuntime&);
decltype(PlayerPositionCallees::hold) bind_player_hold(PlayerHoldRuntime&&)=delete;
decltype(PlayerPositionCallees::hold) bind_player_hold(const PlayerHoldRuntime&&)=delete;
} // namespace sadx
