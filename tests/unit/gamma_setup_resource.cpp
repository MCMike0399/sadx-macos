#include "player/gamma_setup_resource.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace {
using namespace sadx;
using namespace sadx::sprite_registry;
bool fail_allocation = false;
unsigned allocations = 0;
// NOTSA: independent canonical pack serialization for inverse qualification.
void word(std::vector<std::uint8_t> &bytes, std::uint32_t value, unsigned size = 4) {
    for (unsigned index = 0; index < size; ++index)
        bytes.push_back(static_cast<std::uint8_t>(value >> (8 * index)));
}
void scalar(std::vector<std::uint8_t> &bytes, float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, 4);
    word(bytes, bits);
}
std::vector<std::uint8_t> encode(const gamma_setup::Asset &asset) {
    std::vector<std::uint8_t> bytes{'S', 'D', 'X', 'G', 'S', 'P', '0', '1'};
    word(bytes, static_cast<std::uint32_t>(asset.definition.textures->entries.size()));
    const auto &definition = asset.definition;
    word(bytes, definition.animation_count, 2);
    word(bytes, definition.count_padding, 2);
    word(bytes, definition.source_blend);
    word(bytes, definition.destination_blend);
    const auto &parameters = asset.parameters;
    scalar(bytes, parameters.leading_parameter);
    scalar(bytes, parameters.scale_growth);
    scalar(bytes, parameters.animation_speed);
    scalar(bytes, parameters.drag);
    scalar(bytes, parameters.vertical_acceleration);
    for (const auto value : parameters.remaining) scalar(bytes, value);
    for (const auto &frame : definition.animation) {
        for (const auto value : {frame.width, frame.height, frame.center_x, frame.center_y,
                                 frame.u_begin, frame.v_begin, frame.u_end, frame.v_end,
                                 frame.texture_index, frame.attributes})
            word(bytes, static_cast<std::uint16_t>(value), 2);
    }
    return bytes;
}
}
namespace sadx::sprite_registry {
TaskWorkResult allocate_zeroed(TaskWorkArena &arena, std::uint32_t count, std::uint32_t size) {
    ++allocations;
    if (fail_allocation) return {TaskWorkFailure::Exhausted, {}};
    const auto extent = ((size + 3) & ~std::uint32_t{3}) * count;
    auto result = arena.allocate(extent);
    if (result) result = arena.fill(result.work, 0, 0, extent);
    return result;
}
}
int main(int argc, char **argv) {
    using namespace sadx;
    texture_lifetime::List textures;
    std::array<texture_lifetime::Memory, 12> memory{};
    for (auto &entry : memory) textures.entries.push_back(&entry);
    gamma_setup::Asset input{};
    input.definition.textures = &textures;
    input.definition.animation_count = 8;
    input.definition.source_blend = 8;
    input.definition.destination_blend = 10;
    input.definition.animation.resize(8);
    input.definition.animation[0] = {-32768, 32767, -1, 1, 2, 3, 4, 5, 6, 7};
    auto bytes = encode(input);
    if (argc == 2) {
        std::ifstream stream(argv[1], std::ios::binary);
        assert(stream.good());
        bytes.assign(std::istreambuf_iterator<char>(stream), {});
    }
    const auto asset = gamma_setup::decode(bytes, textures);
    assert(encode(asset) == bytes);
    assert(asset.definition.textures == &textures);
    assert(asset.definition.textures->entries[5] == &memory[5]);
    for (unsigned size = 0; size < bytes.size(); ++size) {
        const std::vector<std::uint8_t> truncated(bytes.begin(), bytes.begin() + size);
        try {
            (void)gamma_setup::decode(truncated, textures);
            assert(false);
        } catch (const std::runtime_error &) {}
    }
    for (unsigned offset : {0U, 8U, 12U, 14U}) {
        auto corrupt = bytes;
        corrupt[offset] ^= 1;
        try {
            (void)gamma_setup::decode(corrupt, textures);
            assert(false);
        } catch (const std::runtime_error &) {}
    }
    texture_lifetime::List wrong;
    try {
        (void)gamma_setup::decode(bytes, wrong);
        assert(false);
    } catch (const std::runtime_error &) {}
    TaskWorkArena arena;
    sprite_registry::State registry;
    gamma_setup::State state{&asset.definition, {}};
    gamma_setup::load_resource(state, registry, arena);
    const auto first = state.sprite;
    assert(first && first == registry.head && registry.count == 1);
    assert(first->definition == &asset.definition && !first->extra_data.allocation);
    assert(first->allocation->requested_size() == 28);
    gamma_setup::load_resource(state, registry, arena);
    assert(state.sprite != first && state.sprite->next == first && registry.count == 2);
    assert(!first->allocation->released());
    const auto head = registry.head;
    fail_allocation = true;
    gamma_setup::load_resource(state, registry, arena);
    assert(!state.sprite && registry.head == head && registry.count == 2);
    assert(!head->allocation->released());
    state.definition = nullptr;
    const auto previous_calls = allocations;
    gamma_setup::load_resource(state, registry, arena);
    assert(!state.sprite && allocations == previous_calls && registry.count == 2);
    assert(first->definition->animation.size() == 8);
}
