#include "engine/sprite_registry.hpp"
#include <array>
#include <cstring>

namespace {
using namespace sadx;
using namespace sadx::sprite_registry;
State *live;
NodeHandle replacement;
std::uint32_t post_count, calls, argument_count, argument_size;
bool fail;
#ifdef SADX_SPRITE_TRACE
std::uint32_t *prefix_output;
std::uint32_t old_head_identity, prefix_count;
#endif
}
namespace sadx::sprite_registry {
#ifdef SADX_SPRITE_TRACE
void record_prefix(unsigned phase, const State &state, const NodeHandle &node) {
    auto out = prefix_output + 14 + prefix_count++ * 6;
    out[0] = phase;
    out[1] = state.head == node ? 1 : old_head_identity;
    out[2] = state.count;
    out[3] = node->next == replacement ? old_head_identity : 0;
    // Only the fields already stored by the constructor are observed. Before
    // those stores wrapped-small allocations may still contain arena fill.
    out[4] = phase == 4 ? static_cast<std::uint32_t>(
                             reinterpret_cast<std::uintptr_t>(node->definition)) : 0;
    out[5] = phase >= 3 && node->extra_data.allocation ? node->extra_data.byte_offset : 0;
}
#endif
TaskWorkResult allocate_zeroed(TaskWorkArena &arena, std::uint32_t count, std::uint32_t size) {
    ++calls;
    argument_count = count;
    argument_size = size;
    live->count = post_count;
    live->head = replacement;
    if (fail)
        return {TaskWorkFailure::exhausted, {}};
    const auto bytes = ((size + 3) & ~std::uint32_t{3}) * count;
    auto result = arena.allocate(bytes);
    if (result)
        result = arena.fill(result.work, 0, 0, bytes);
    return result;
}
}
// Input: definition identity, extra bytes, initial count, initial head identity,
// post-allocation count, post-allocation head identity, forced allocation failure.
extern "C" int sadx_native_sprite_registry(const std::uint32_t *in, std::uint32_t *out) {
    if (!in || !out)
        return 0;
    using namespace sadx;
    using namespace sadx::sprite_registry;
    TaskWorkArena arena;
    State state;
    auto before = in[3] ? std::make_shared<Node>() : NodeHandle{};
    replacement = in[5] ? std::make_shared<Node>() : NodeHandle{};
    state.head = before;
    state.count = in[2];
    live = &state;
    post_count = in[4];
    fail = in[6] != 0;
    calls = argument_count = argument_size = 0;
    const auto definition = reinterpret_cast<const Definition *>(std::uintptr_t(in[0]));
#ifdef SADX_SPRITE_TRACE
    prefix_output = out;
    prefix_count = 0;
    old_head_identity = in[5];
#endif
    const auto node = register_sprite(state, arena, definition, in[1]);
    out[0] = calls;
    out[1] = argument_count;
    out[2] = argument_size;
    out[3] = state.count;
    out[4] = node ? 1 : (state.head == before ? in[3] : in[5]);
    out[5] = node ? 1 : 0;
    std::array<std::uint32_t, 7> bytes{};
    if (node) {
        if (!arena.read(node->allocation, 0, bytes.data(), 28))
            return 0;
        // Unchanged zero bytes are read from actual arena storage, not fabricated.
        bytes[0] = node->next == replacement ? in[5] : in[3];
        bytes[4] = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(node->definition));
        bytes[6] = node->extra_data.allocation == node->allocation ? node->extra_data.byte_offset : 0;
        if (node->extra_data.allocation && node->extra_data.byte_offset != 28)
            return 0;
        out[13] = node->allocation->requested_size();
    } else {
        out[13] = 0;
    }
    std::memcpy(out + 6, bytes.data(), 28);
#ifdef SADX_SPRITE_TRACE
    out[44] = prefix_count;
#endif
    live = nullptr;
    replacement.reset();
    return 1;
}
