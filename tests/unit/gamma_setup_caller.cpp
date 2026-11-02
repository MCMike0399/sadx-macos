// Reuse the unchanged Gamma caller fixture and its real scheduler/PInitialize.
// Only setup is replaced with this packet; the first material consumption stops.
#define load_setup_resource fixture_setup_receipt
#define material fixture_material
#include "../probes/native_gamma_selection_core_probe.cpp"
#undef material
#undef load_setup_resource
#include "player/gamma_setup_resource.hpp"
#include <cassert>
#include <fstream>
#include <iterator>

namespace {
sadx::gamma_setup::State setup;
sadx::sprite_registry::State sprites;
struct NextMaterial {};
}
namespace sadx::gamma_selection {
void load_setup_resource(Context &context) {
    gamma_setup::load_resource(setup, sprites, *context.arena);
    context.receipt(12);
}
SceneMaterial &material(Context &, unsigned, unsigned) {
    throw NextMaterial{};
}
}
int main(int argc, char **argv) {
    using namespace sadx;
    assert(argc == 2);
    std::ifstream stream(argv[1], std::ios::binary);
    assert(stream.good());
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(stream), {}};
    texture_lifetime::List textures;
    textures.entries.resize(12);
    const auto asset = gamma_setup::decode(bytes, textures);
    setup.definition = &asset.definition;
    std::array<std::uint32_t, 32> input{};
    input[2] = 2;
    input[26] = gamma_selection::bits(15);
    input[31] = 255;
    gamma_selection::Context context(input.data(), 127);
    context.native_initialize = true;
    context.parameters[6].rad = 5;
    context.parameters[6].height = 15;
    const auto primary = context.task->primary_state;
    const auto motion = context.task->motion_state;
    for (unsigned index = 0; index < 2; ++index) {
        player_initialization::bind_primary_view(context.task).mode = 0;
        try {
            gamma_selection::run(context, context.task);
            assert(false);
        } catch (const NextMaterial &) {}
        assert(context.task->primary_state == primary && context.task->motion_state == motion);
        assert(context.live && context.registry.players[0] == context.live);
        assert(sprites.count == index + 1 && sprites.head == setup.sprite);
        assert(setup.sprite->definition == &asset.definition);
        assert(setup.sprite->allocation && !setup.sprite->allocation->released());
        assert(player_initialization::bind_primary_view(context.task).mode == 1);
    }
    assert(setup.sprite->next && setup.sprite->next->definition == &asset.definition);
    assert(!setup.sprite->next->allocation->released());
}
