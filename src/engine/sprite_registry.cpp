#include "engine/sprite_registry.hpp"

namespace sadx::sprite_registry {
NodeHandle register_sprite(State &state, TaskWorkArena &arena, const Definition *definition,
                           std::uint32_t extra_bytes) {
    if (!definition)
        return {};
    const auto allocation = allocate_zeroed(arena, 1, extra_bytes + 28);
    if (!allocation)
        return {};

    // Read the live globals after CAlloc: allocation can reenter this system.
    const auto count = state.count;
    const auto old_head = state.head;
    auto node = std::make_shared<Node>();
    node->allocation = allocation.work;
    node->next = old_head;
    state.head = node;
    state.count = count + 1;
    // Original quirk: TEST clears carry, so JBE skips only zero, including
    // when the caller supplied a negative extra size in the original ABI.
    if (extra_bytes != 0)
        node->extra_data = {allocation.work, 28};
    node->definition = definition;
    return node;
}
} // namespace sadx::sprite_registry
