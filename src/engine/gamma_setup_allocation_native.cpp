#include "engine/sprite_registry.hpp"
#include <stdexcept>

namespace sadx::sprite_registry {
// NOTSA: bind the existing CAlloc seam to the canonical native task arena.
// Source rounds the element size before wrapped multiplication, then clears
// the entire allocated extent. No registry-private pool or source heap is added.
TaskWorkResult allocate_zeroed(TaskWorkArena &arena, std::uint32_t count, std::uint32_t size) {
    constexpr std::uint32_t kWordAlignmentMask = 3;
    const auto extent = ((size + kWordAlignmentMask) & ~kWordAlignmentMask) * count;
    const auto allocation = arena.allocate(extent);
    if (!allocation)
        return allocation;
    const auto cleared = arena.fill(allocation.work, 0, 0, extent);
    if (!cleared)
        throw std::runtime_error("native_gamma_setup_CAlloc_zero_fill");
    return allocation;
}
} // namespace sadx::sprite_registry
