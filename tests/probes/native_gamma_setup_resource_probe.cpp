#include "player/gamma_setup_resource.hpp"
#include <algorithm>

namespace {
using namespace sadx;
using namespace sadx::sprite_registry;
gamma_setup::State *gamma;
State *registry;
const std::uint32_t *input;
std::vector<NodeHandle> nodes;
NodeHandle marker;
std::uint32_t calls, observed_result, observed_definition, allocation_count, allocation_size;
std::uint32_t tag(const NodeHandle &node) {
    if (!node) return 0;
    if (node == marker) return 0x22000000;
    for (unsigned i = 0; i < nodes.size(); ++i)
        if (node == nodes[i]) return 0x11000000 + 256 * i;
    return 0x20000000;
}
}
namespace sadx::sprite_registry {
TaskWorkResult allocate_zeroed(TaskWorkArena &arena, std::uint32_t count, std::uint32_t size) {
    ++calls;
    observed_result = tag(gamma->sprite);
    observed_definition = gamma->definition ? 0x9820f8 : 0;
    allocation_count = count;
    allocation_size = size;
    if (input[3]) {
        registry->count = input[4];
        registry->head = input[6] ? marker : NodeHandle{};
        gamma->sprite = marker;
    }
    if (input[8]) gamma->definition = nullptr;
    if (input[1]) return {TaskWorkFailure::Exhausted, {}};
    const auto extent = ((size + 3) & ~std::uint32_t{3}) * count;
    auto result = arena.allocate(extent);
    if (result) result = arena.fill(result.work, 0, 0, extent);
    return result;
}
}
extern "C" int gamma_setup_case(const std::uint32_t *values, std::uint32_t *output) {
    input = values;
    TaskWorkArena arena;
    State sprites;
    gamma_setup::State state;
    Definition definition{};
    marker = std::make_shared<Node>();
    nodes.clear();
    calls = observed_result = observed_definition = allocation_count = allocation_size = 0;
    sprites.count = input[2];
    sprites.head = input[5] ? std::make_shared<Node>() : NodeHandle{};
    state.sprite = marker;
    state.definition = input[0] ? &definition : nullptr;
    gamma = &state;
    registry = &sprites;
    for (unsigned index = 0; index < input[7]; ++index) {
        gamma_setup::load_resource(state, sprites, arena);
        if (state.sprite) nodes.push_back(state.sprite);
        auto *row = output + index * 14;
        const auto &node = state.sprite;
        const std::uint32_t result[] = {calls, sprites.count, tag(sprites.head), tag(node),
            node ? tag(node->next) : 0, node && node->definition ? 0x9820f8U : 0,
            node && node->extra_data.allocation ? node->extra_data.byte_offset : 0,
            node && node->allocation ? node->allocation->requested_size() : 0,
            observed_result, observed_definition, state.definition ? 0x9820f8U : 0,
            allocation_count, allocation_size, node && node->next && node->next->allocation &&
                node->next->allocation->released() ? 1U : 0U};
        std::copy(std::begin(result), std::end(result), row);
    }
    nodes.clear();
    marker.reset();
    return 1;
}
