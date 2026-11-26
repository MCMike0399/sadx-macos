#pragma once

#include "engine/character_pack.hpp"
#include "engine/task_work.hpp"
#include "math/ninja_math.hpp"
#include <vector>

namespace sadx {
// playerwk +0x14c..+0x1d8. PJoinVertexes modes 4..9 write one point/vector
// pair per attachment; display code consumes them later.
struct PlayerAttachmentVectors {
    NinjaVector right_hand_position{};   // playerwk +0x14c
    NinjaVector left_hand_position{};    // +0x158
    NinjaVector right_foot_position{};   // +0x164
    NinjaVector left_foot_position{};    // +0x170
    NinjaVector user0_position{};        // +0x17c
    NinjaVector user1_position{};        // +0x188
    NinjaVector right_hand_direction{};  // +0x194
    NinjaVector left_hand_direction{};   // +0x1a0
    NinjaVector right_foot_direction{};  // +0x1ac
    NinjaVector left_foot_direction{};   // +0x1b8
    NinjaVector user0_direction{};       // +0x1c4
    NinjaVector user1_direction{};       // +0x1d0
};

// PL_JOIN_VERTEX. Objects borrow the scene's stable, mutable imported identities.
struct PlayerJoinVertex {
    character_pack::ObjectResource *object = nullptr;
    character_pack::ObjectResource *source_object = nullptr;
    character_pack::ObjectResource *destination_object = nullptr;
    std::uint8_t vertex_count = 0;
    std::int8_t input_mode = 0;
    std::uint8_t source_depth = 0;
    std::uint8_t destination_depth = 0;
    // org owns the actual MAlloc block: four point/normal triples (48 bytes)
    // per vertex. No backup values exist outside that allocation.
    TaskWorkHandle original_vertices;
    const std::vector<std::uint16_t> *vertex_pairs = nullptr;
};
struct PlayerJoinTable {
    std::vector<PlayerJoinVertex> rows;
};
} // namespace sadx
