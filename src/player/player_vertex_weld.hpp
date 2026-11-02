#pragma once

#include "engine/character_geometry.hpp"
#include "player/player_join_vertices.hpp"
#include <functional>
#include <stdexcept>

namespace sadx::vertex_weld {
enum class Boundary {
    TableRow,
    ObjectModel,
    VertexPair,
    VectorRead,
    VectorWrite,
    BackupRead,
    BackupWrite,
    ReleaseBackup,
    NodeRead
};
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, std::size_t row, unsigned vertex, unsigned component,
          TaskWorkFailure failure = TaskWorkFailure::None)
        : std::runtime_error("player vertex weld memory boundary"), boundary(boundary), row(row),
          vertex(vertex), component(component), work_failure(failure) {}
    Boundary boundary;
    std::size_t row;
    unsigned vertex;
    unsigned component;
    TaskWorkFailure work_failure;
};

// FUNCTION: SONIC 0x439f60 GetNodeIndex
int node_index(const character_pack::ObjectResource *root,
               const character_pack::ObjectResource *target, const character_pack::ObjectState &);

// NOTSA: recovered PJoinVertexes dispatch, with lazy native operations so active
// matrix processing precedes table consumption. The active operation still stops
// at its unrecovered boundary; initialization publishes mode only on return.
void update_prefix(std::int16_t &mode, const std::function<void()> &process_active,
                   const std::function<void()> &initialize_table,
                   const std::function<void()> &restore_table);

// NOTSA: source initialization branch split from PJoinVertexes. The caller stores
// join mode 1 only after the whole table has returned, including the sentinel read.
void initialize(PlayerJoinTable &, TaskWorkArena &, character_pack::SceneState &);
// NOTSA: source cleanup branch split from PJoinVertexes. Restoration reads the
// actual arena backup and writes the same mutable arrays consumed by model draws.
void restore(PlayerJoinTable &, TaskWorkArena &, character_pack::SceneState &);
} // namespace sadx::vertex_weld
