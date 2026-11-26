#pragma once

#include "engine/character_geometry.hpp"
#include "engine/ninja_matrix.hpp"
#include "player/model_motion_matrices.hpp"
#include "player/player_join_vertices.hpp"
#include "player/player_physics.hpp"
#include <functional>
#include <memory>
#include <stdexcept>

namespace sadx::animation {
struct Resources;
}
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
    NodeRead,
    MotionSource,
    MatrixTransform
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

// NOTSA: the four matrix globals PJoinVertexes owns. GetMMMatrix destinations
// persist when a rejected index leaves them unchanged, so the caller owns one
// shared instance for the whole player system.
struct Matrices {
    // GLOBAL: SONIC 0x3b42f38 WeldSourceMatrix size=64
    NinjaMatrix source;
    // GLOBAL: SONIC 0x3b42eb8 WeldDestinationMatrix size=64
    NinjaMatrix destination;
    // GLOBAL: SONIC 0x3b42ef8 WeldSourceInverse size=64
    NinjaMatrix source_inverse;
    // GLOBAL: SONIC 0x3b42e78 WeldDestinationInverse size=64
    NinjaMatrix destination_inverse;
};

// NOTSA: the source's matrix/transform calls. The native adapter forwards to
// NinjaMatrixPrimitives and the recovered matrix owners; the differential
// oracle substitutes scripted fixture events because those owners have their
// own oracles.
class PrimitiveOperations {
  public:
    virtual ~PrimitiveOperations() = default;
    virtual void set_matrix(NinjaMatrix &, const NinjaMatrix &) = 0;
    virtual void invert_matrix(NinjaMatrix &) = 0;
    virtual void multiply_current(const NinjaMatrix &) = 0;
    virtual bool push_matrix(const NinjaMatrix &) = 0;
    virtual void pop_matrix() = 0;
    virtual NinjaMatrix &current_matrix() = 0;
    virtual void calculate_point(const NinjaMatrix &, const NinjaVector &, NinjaVector &) = 0;
    virtual void calculate_vector(const NinjaMatrix &, const NinjaVector &, NinjaVector &) = 0;
    virtual void unit_vector(NinjaVector &) = 0;
};

// NOTSA: CalcMMMatrix/GetMMMatrix/GetNodeIndex as the active weld consumes
// them. Production forwards to model_matrices and vertex_weld::node_index.
class MotionMatrices {
  public:
    virtual ~MotionMatrices() = default;
    virtual void calculate(float frame, std::uint32_t node_count) = 0;
    virtual bool get(std::uint32_t index, NinjaMatrix &destination) = 0;
    virtual int node_index(const character_pack::ObjectResource *root,
                           const character_pack::ObjectResource *target) = 0;
    virtual const character_pack::ObjectResource *action_object() = 0;
};

// NOTSA: recovered PJoinVertexes dispatch, with lazy native operations so active
// matrix processing precedes table consumption. Initialization publishes mode
// only on return.
void update_prefix(std::int16_t &mode, const std::function<void()> &process_active,
                   const std::function<void()> &initialize_table,
                   const std::function<void()> &restore_table);

// NOTSA: source initialization branch split from PJoinVertexes. The caller stores
// join mode 1 only after the whole table has returned, including the sentinel read.
void initialize(PlayerJoinTable &, TaskWorkArena &, character_pack::SceneState &);
// NOTSA: source cleanup branch split from PJoinVertexes. Restoration reads the
// actual arena backup and writes the same mutable arrays consumed by model draws.
void restore(PlayerJoinTable &, TaskWorkArena &, character_pack::SceneState &);

// NOTSA: active (mode 1) branch. The motion matrices and row transform order are
// exactly the source's: CalcMMMatrix, the root-object gate, GetMMMatrix, then
// the destination-null point/vector dispatch or the vertex blending cases.
void process_active(PlayerJoinTable &, TaskWorkArena &, character_pack::SceneState &,
                    PlayerAttachmentVectors &, Matrices &, float frame,
                    std::uint32_t node_count, MotionMatrices &, PrimitiveOperations &,
                    fidelity::Context &);

// NOTSA: resolve the current PL_ACTION row (or the generated mtnmode-2 buffer)
// into the motion source CalcMMMatrix scans. Null means a required source field
// or declaration is absent; the caller reports its own actor stop.
std::unique_ptr<model_matrices::MotionSource>
active_motion_source(PlayerPhysicsState &, TaskWorkArena &, const animation::Resources &,
                     const character_pack::ObjectState &, float &frame,
                     std::uint32_t &node_count);

// NOTSA: one production entry for the active branch. False means the source
// action/motion row is absent; memory and transform failures still throw Error.
bool run_active(PlayerJoinTable &, TaskWorkArena &, character_pack::SceneState &,
                PlayerPhysicsState &, model_matrices::State &, NinjaMatrixPrimitives &,
                const animation::Resources &, Matrices &, fidelity::Context &);

// NOTSA: production adapters over the recovered numeric owners.
std::unique_ptr<PrimitiveOperations> native_operations(NinjaMatrixPrimitives &,
                                                       fidelity::Context &);
std::unique_ptr<MotionMatrices> native_motion_matrices(model_matrices::State &,
    NinjaMatrixPrimitives &, model_matrices::MotionSource &,
    const character_pack::ObjectState &, fidelity::Context &);
} // namespace sadx::vertex_weld
