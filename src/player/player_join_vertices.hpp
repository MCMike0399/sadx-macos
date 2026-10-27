#pragma once

#include "engine/character_pack.hpp"
#include "engine/task_work.hpp"
#include <vector>

namespace sadx {
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
