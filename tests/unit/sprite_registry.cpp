#include "engine/sprite_registry.hpp"
#include <cassert>
#include <limits>

namespace {
using namespace sadx;
using namespace sadx::sprite_registry;
State *state;
NodeHandle old_head;
std::uint32_t calls = 0;
bool fail = false;
}
namespace sadx::sprite_registry {
TaskWorkResult allocate_zeroed(TaskWorkArena &arena, std::uint32_t count, std::uint32_t size) {
    ++calls;
    assert(count == 1);
    state->head = old_head;
    state->count = std::numeric_limits<std::uint32_t>::max();
    if (fail)
        return {TaskWorkFailure::exhausted, {}};
    const auto bytes = ((size + 3) & ~std::uint32_t{3}) * count;
    auto allocation = arena.allocate(bytes);
    if (allocation)
        allocation = arena.fill(allocation.work, 0, 0, bytes);
    return allocation;
}
}
int main() {
    TaskWorkArena arena;
    State registry;
    state = &registry;
    old_head = std::make_shared<Node>();
    const int token = 1;
    const auto definition = reinterpret_cast<const Definition *>(&token);
    assert(!register_sprite(registry, arena, nullptr, 0));
    assert(calls == 0 && !registry.head && registry.count == 0);
    auto node = register_sprite(registry, arena, definition, 0);
    assert(node && registry.head == node && registry.count == 0);
    assert(node->next == old_head && node->definition == definition);
    assert(!node->extra_data.allocation);
    std::uint8_t bytes[28];
    assert(arena.read(node->allocation, 0, bytes, 28));
    for (auto byte : bytes)
        assert(byte == 0);
    node = register_sprite(registry, arena, definition, 13);
    assert(node && node->extra_data.allocation == node->allocation);
    assert(node->extra_data.byte_offset == 28 && node->allocation->requested_size() == 44);
    std::uint8_t value = 71, read = 0;
    assert(arena.write(node->extra_data.allocation, node->extra_data.byte_offset, &value, 1));
    assert(arena.read(node->allocation, 28, &read, 1) && read == value);
    fail = true;
    assert(!register_sprite(registry, arena, definition, 8));
    assert(registry.head == old_head && registry.count == std::numeric_limits<std::uint32_t>::max());
    fail = false;
    node = register_sprite(registry, arena, definition, std::uint32_t(-24));
    assert(node && node->allocation->requested_size() == 4);
    assert(node->extra_data.allocation == node->allocation && node->extra_data.byte_offset == 28);
    assert(!register_sprite(registry, arena, definition, std::uint32_t(-28)));
    assert(registry.head == old_head && registry.count == std::numeric_limits<std::uint32_t>::max());
}
