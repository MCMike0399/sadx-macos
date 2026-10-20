#include "game/game_bootstrap.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace {
struct World {
    std::int32_t card_result = 8;
    std::uint32_t global_timer = 123, movie = 4;
    std::int16_t rings = 19;
    std::uint16_t mode = 0;
    sadx::bootstrap::State state{{card_result, global_timer, rings, movie, mode}};
    sadx::save::Data save;
};
}
int main() {
    using namespace sadx;
    World world;
    world.state.character = 7;
    world.save.options = 2;
    std::int16_t stage = 25;
    unsigned calls = 0;
    bootstrap::Services services{
        [&](std::uint16_t old_character) {
            assert(++calls == 1 && old_character == 7);
            assert(world.state.card_state == 1 && world.card_result == 0);
            assert(world.state.character == 7 && world.global_timer == 123);
            world.save.lives[0] = 127;
        },
        [&](std::uint32_t port) {
            assert(++calls == 2 && port == 0);
            assert(world.state.character == 0 && world.state.lives == 127);
            assert(world.mode == 21 && world.movie == 0 && world.rings == 0);
            assert(world.state.saved_option_gate == 0 && world.state.gravity[1] == -1);
            assert(world.state.sequence_variables[65] == 1);
            stage = 26;
        }};
    bootstrap::start(world.state, world.save, {{4294967296ULL}, {8589934592ULL}}, stage, services);
    assert(calls == 2 && world.state.random_seed == 3735882923U);
    assert(world.global_timer == 0 && world.state.actions.clear_pose.identity == 4294967296ULL);
    assert(world.state.actions.clear_pose_transition.identity == 8589934592ULL);
    assert(world.state.material_callback == bootstrap::MaterialCallback::CorrectMaterial);
    for (const auto &record : world.state.character_work.records) {
        assert(record[0] == 8);
        for (unsigned i = 1; i < record.size(); ++i)
            assert(record[i] == 0);
    }
    stage = 25;
    services.stop_vibration = [&](std::uint32_t) { stage = 26; };
    assert(bootstrap::initialize_game(world.state, world.save, stage, services) == 1);
    World stopped;
    stopped.state.character = 7;
    stopped.state.perfect_chaos_debug = 3;
    stopped.state.random_seed = 42;
    bootstrap::Services failing{[](std::uint16_t) {}, [](std::uint32_t) {
        throw std::runtime_error("vibration service unavailable");
    }};
    bool failed = false;
    try {
        bootstrap::start(stopped.state, stopped.save, {}, stage, failing);
    } catch (const std::runtime_error &) {
        failed = true;
    }
    assert(failed && stopped.mode == 21 && stopped.rings == 0);
    assert(stopped.state.material_callback == bootstrap::MaterialCallback::None);
    assert(stopped.state.random_seed == 42 && stopped.state.perfect_chaos_debug == 3);
    World early;
    early.state.character = 7;
    failing.unload_character_textures = [](std::uint16_t) {
        throw std::runtime_error("texture release unavailable");
    };
    failed = false;
    try {
        bootstrap::start(early.state, early.save, {}, stage, failing);
    } catch (const std::runtime_error &) {
        failed = true;
    }
    assert(failed && early.state.card_state == 1 && early.card_result == 0);
    assert(early.state.character == 7 && early.global_timer == 123 && early.rings == 19);
    std::uint8_t menu_mode = 9;
    world.state.menu.flags.fill(165);
    world.state.menu.selected_storage = 3;
    world.state.menu.selected_file = 2;
    world.state.menu.character_cursor = 1;
    world.state.menu.player_number = 4;
    bootstrap::initialize_advertise(world.state.menu, menu_mode);
    assert(menu_mode == 0 && world.state.menu.selected_storage == 0);
    assert(world.state.menu.flags[7] == 0 && world.state.menu.player_number == 0);
    assert(bootstrap::loading_status() == 15 && bootstrap::loading_ready() == 1);
    std::cout << "Native game Start preserves resource identities, shared owners and service failure prefixes\n";
}
