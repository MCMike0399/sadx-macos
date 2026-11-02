#include "player/gamma_setup_resource.hpp"
#include <cassert>

int main() {
    using namespace sadx;
    TaskWorkArena arena;
    const auto allocation = sprite_registry::allocate_zeroed(arena, 2, 5);
    assert(allocation && allocation.work->requested_size() == 16);
    std::array<std::uint8_t, 16> bytes{};
    assert(arena.read(allocation.work, 0, bytes.data(), bytes.size()));
    for (const auto byte : bytes) assert(byte == 0);
    const auto wrapped = sprite_registry::allocate_zeroed(arena, 2, (1U << 31) + 1);
    assert(wrapped && wrapped.work->requested_size() == 8);
    assert(!sprite_registry::allocate_zeroed(arena, 1, std::uint32_t(-1)));
    assert(!sprite_registry::allocate_zeroed(arena, 0, 28));
    sprite_registry::State registry;
    sprite_registry::Definition definition{};
    gamma_setup::State gamma{&definition, {}};
    gamma_setup::load_resource(gamma, registry, arena);
    const auto first = gamma.sprite;
    assert(first && first->definition == &definition);
    // Exhaust this actual arena through its public allocator. Failed setup must
    // overwrite the Gamma result with NULL while retaining the registry node.
    while (arena.allocate(64)) {}
    while (arena.allocate(1)) {}
    gamma_setup::load_resource(gamma, registry, arena);
    assert(!gamma.sprite && registry.count == 1 && registry.head == first);
    assert(!first->allocation->released());
}
