// Compile only against the copied Root owner contract, not an admitted runtime.
#include "runtime/native_session.hpp"
#include "runtime/native_display.hpp"
#include <cassert>
#include <iostream>
#include <cstring>
namespace sadx::native {
struct DisplayObjectProbe {
    static const auto &slots(const Display &display) { return display.matrices_.slots(); }
};
}
int main(int argc, char **argv) {
    using namespace sadx;
    assert(argc == 2);
    const std::filesystem::path directory(argv[1]);
    auto pack = character_pack::CharacterPack::load(directory / "characters/sonic-selection.actions");
    character_pack::SceneState scene(pack);
    native::PlayerSystem players;
    TaskScheduler scheduler;
    TaskHandle task;
    std::int32_t card = 0;
    std::uint32_t timer = 0, movie = 0;
    std::int16_t rings = 0, stage = 0, act = 0;
    std::uint16_t mode = 0;
    bootstrap::State startup({card, timer, rings, movie, mode});
    frame::ModeState timing;
    native::Display display(directory / "system", timing);
    sprite_registry::State sprites;
    gamma_selection::Context context{players, scheduler, startup, display, task, stage, act,
        nullptr, nullptr, nullptr, {nullptr, nullptr, 0}, nullptr, nullptr, sprites, scene};
    model_matrices::State state;
    context.model_matrices = &state;
    state.count = 128;
    state.limit = 97;
    for (unsigned node = 0; node < 128; ++node)
        for (unsigned component = 0; component < 16; ++component)
            state.matrices[node][component] = float(node * 16 + component + 1);
    const auto snapshot = state.matrices;
    for (unsigned node : {54U, 30U, 39U, 2U, 37U, 126U, 127U}) {
        NinjaMatrix destination{};
        gamma_selection::get_model_matrix(context, node, destination);
        assert(destination == state.matrices[node]);
    }
    NinjaMatrix destination = state.matrices[17];
    const auto previous = destination;
    for (unsigned node : {128U, 0x80000000U, 0xffffffffU}) {
        gamma_selection::get_model_matrix(context, node, destination);
        assert(destination == previous);
    }
    assert(state.matrices == snapshot && state.count == 128 && state.limit == 97);
    model_matrices::State other;
    other.count = 2;
    other.limit = 128;
    for (unsigned component = 0; component < 16; ++component)
        other.matrices[1][component] = float(5000 + component);
    context.model_matrices = &other;
    gamma_selection::get_model_matrix(context, 1, destination);
    assert(destination == other.matrices[1] && state.matrices == snapshot);
    context.model_matrices = &state;
    destination = previous;
    state.count = 0;
    gamma_selection::get_model_matrix(context, 54, destination);
    assert(destination == previous);
    context.model_matrices = nullptr;
    bool rejected = false;
    try { gamma_selection::get_model_matrix(context, 0, destination); }
    catch (const flow::UnrecoveredGameFlowCall &error) {
        rejected = std::string(error.what()) == "native_gamma_get_model_matrix";
    }
    assert(rejected);
    const auto stack_before = native::DisplayObjectProbe::slots(display);
    NinjaMatrix rotated{};
    for (unsigned component = 0; component < 16; ++component)
        rotated[component] = float(component + 1);
    gamma_selection::rotate_z(context, rotated, 16384);
    const NinjaMatrix expected{5, 6, 7, 8, -1, -2, -3, -4, 9, 10, 11, 12, 13, 14, 15, 16};
    assert(std::memcmp(rotated.data(), expected.data(), 64) == 0);
    rotated = {};
    gamma_selection::rotate_z(context, rotated, 16384);
    NinjaMatrix zero_expected{};
    for (unsigned component = 4; component < 8; ++component)
        zero_expected[component] = -0.0f;
    assert(std::memcmp(rotated.data(), zero_expected.data(), 64) == 0);
    assert(native::DisplayObjectProbe::slots(display) == stack_before);
    std::cout << "Root getter contract: 112 nonzero components, distinct live owners, unsigned rejection, count/limit/buffer retention and missing owner, explicit-matrix rotation/signed zeros and stack retention pass\n";
}
