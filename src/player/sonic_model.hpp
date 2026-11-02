#pragma once
#include "engine/character_pack.hpp"
#include "engine/task.hpp"
#include "player/player_join_vertices.hpp"
#include <array>

namespace sadx::sonic_model {
using ObjectId = character_pack::ResourceId;
using DirectAheadModels = std::array<ObjectId, 3>;
// NOTSA: session-lifetime storage for the source's shared Sonic model state.
// Imported objects remain owned by the character scene, including sibling edits.
struct State {
    // GLOBAL: SONIC 0x3c55e28 SonicWeldInfo size=888
    PlayerJoinTable joins;
    // GLOBAL: SONIC 0x3c55d68 MetalSonicWeldInfo size=192
    PlayerJoinTable metal_joins;
    // GLOBAL: SONIC 0x3c55d58 SonicDirectAheadModels size=12
    DirectAheadModels direct_ahead = {};
    // GLOBAL: SONIC 0x3c55d40 SonicModelSibling size=4
    ObjectId model_sibling = 0;
};
// NOTSA: references into the actual initialized player and primary owners.
struct Player {
    PlayerJoinTable *&joins;
    std::int16_t &join_mode;
    std::uint16_t &idle_timer;
    std::uint8_t &mode;
};
// NOTSA: helper composition, not a replacement implementation of either callee.
// PJoinVertexes retains its single provenance declaration in gamma_selection_core.hpp.
struct Calls {
    virtual ~Calls() = default;
    virtual void join_vertices() = 0;
    virtual void initialize_collision() = 0;
    virtual void store_collision_height() = 0;
};
// NOTSA: inline SonicTheHedgehog model initialization prefix, ending before
// LoadCharTextures. This is not a source function entry or a complete actor update.
void initialize_prefix(State &, character_pack::ObjectState &, ObjectId attach, ObjectId root,
                       Player, bool alternate, Calls &);
// NOTSA: source-ordered continuation of the inline mode-zero branch. Concrete
// transports borrow canonical resources and captured player/primary owners.
struct InitializationCalls {
    virtual ~InitializationCalls() = default;
    virtual void load_textures(std::uint8_t character) = 0;
    virtual std::uint8_t metal() = 0;
    virtual void clear_free_word(unsigned index) = 0;
    virtual std::uint8_t player_index() = 0;
    virtual void load_collision_task(std::uint8_t index) = 0;
    virtual void set_motion() = 0;
    virtual std::int16_t stage() = 0;
    virtual std::uint16_t act() = 0;
    virtual character_pack::ObjectResource *join_target() = 0;
    virtual void publish_join_table(PlayerJoinTable &) = 0;
    virtual void join_vertices() = 0;
    virtual void create_effect(unsigned flags) = 0;
    virtual void install_callbacks() = 0;
};
// NOTSA: begins at the first LoadCharTextures and ends after common PSetMotion.
void initialize_resources(State &, InitializationCalls &);

// NOTSA: common-tail model links use live source slots; unavailable imported
// declarations stop at the first consumed slot, before any skipped weld call.
struct CommonTailCalls {
    virtual ~CommonTailCalls() = default;
    virtual std::uint16_t status() = 0;
    virtual std::uint8_t metal() = 0;
    virtual std::uint16_t equipment() = 0;
    virtual ObjectId model_slot(unsigned index) = 0;
    virtual void join_vertices() = 0;
};
// NOTSA: post-PSetMotion prefix; subsequent attachment/sound/display work is open.
void update_common_model_links(character_pack::ObjectState &, CommonTailCalls &);

// NOTSA: the source writes only these three object references in its malloc
// motion block. The rest remains unavailable, including its later algorithm.
struct CollisionTaskWork {
    TaskWorkHandle allocation;
    std::array<ObjectId, 3> objects{};
};
// STUB: SONIC 0x493c70 SonicDirectAhead collision task execution remains unrecovered
void collision_task(TaskScheduler &, const TaskHandle &);
// STUB: SONIC 0x493730 SonicInitializationEffect unrecovered effect callback
void initialize_effect(TaskScheduler &, const TaskHandle &);
// STUB: SONIC 0x4937b0 Sonic_Jiggle_Main jiggle task execution remains unrecovered
void initialize_jiggle(TaskScheduler &, const TaskHandle &);
// STUB: SONIC 0x494860 SonicDestruct actor destruction remains unrecovered
void destroy(TaskScheduler &, const TaskHandle &);
// STUB: SONIC 0x4948c0 SonicDisplay actor drawing remains unrecovered
void display(TaskScheduler &, const TaskHandle &);
// NOTSA: translate the source's null creation result for Sonic initialization.
// Only exhausted allocations with completed rollback are ordinary absence.
TaskHandle create_initialization_task(TaskScheduler &, unsigned flags, int level, TaskCallback);
// FUNCTION: SONIC 0x495860 CollisionCrashThing_Load
bool load_collision_task(const std::array<ObjectId, 3> &, std::uint8_t player, TaskScheduler &,
                         TaskWorkArena &);
} // namespace sadx::sonic_model
