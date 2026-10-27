#pragma once

#include "engine/task_work.hpp"

namespace sadx::sprite_registry {
// The constructor borrows this identity without dereferencing it. Its content layout
// and consumers remain unrecovered; this declaration does not define a payload.
struct Definition;
struct Node;
using NodeHandle = std::shared_ptr<Node>;

struct BufferView {
    TaskWorkHandle allocation;
    std::uint32_t byte_offset = 0;
};
struct Node {
    TaskWorkHandle allocation;
    NodeHandle next;
    const Definition *definition = nullptr;
    BufferView extra_data;
};
struct State {
    // GLOBAL: SONIC 0x3c4858c SpriteRegistryHead size=4
    NodeHandle head;
    // GLOBAL: SONIC 0x3c48590 SpriteCount size=4
    std::uint32_t count = 0;
};

// NOTSA: link-time allocation boundary. Supply the existing arena's CAlloc
// semantics, including zeroing and wrapped size arithmetic. No private pool.
TaskWorkResult allocate_zeroed(TaskWorkArena &, std::uint32_t count, std::uint32_t size);

// FUNCTION: SONIC 0x456c50 RegisterSprite
NodeHandle register_sprite(State &, TaskWorkArena &, const Definition *, std::uint32_t extra_bytes);
} // namespace sadx::sprite_registry
