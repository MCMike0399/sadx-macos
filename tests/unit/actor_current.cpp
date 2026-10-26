#include "player/actor_current.hpp"
#include "player/sonic_player.hpp"
#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <vector>

int main() {
    using namespace sadx;
    static_assert(std::is_same_v<decltype(SonicPlayerGlobals::current), actor_current::State>);
    SonicPlayerGlobals legacy;
    actor_current::State &state = legacy.current;
    assert(!state.primary && !state.player && state.super_sonic == 0);
    TaskWorkArena arena;
    const auto first = arena.allocate(64).work;
    const auto second = arena.allocate(64).work;
    assert(first && second && first != second);
    struct CurrentView {
        TaskWorkHandle &primary;
        TaskWorkHandle &player;
    } view{state.primary, state.player};
    assert(&view.primary == &state.primary && &view.player == &state.player);
    view.primary = first;
    assert(&actor_current::read_primary(state) == &state.primary);
    assert(actor_current::read_primary(state) == first);
    actor_current::set_player(state, first);
    view.primary = second;
    assert(actor_current::read_player(state) == first); // no automatic refresh
    assert(&actor_current::read_player(state) == &state.player);

    std::vector<int> stores;
    actor_current::Observation observation{[&](const TaskWorkHandle &value) {
                                               assert(value == state.primary &&
                                                      state.primary == second);
                                               stores.push_back(1);
                                           },
                                           [&](const TaskWorkHandle &value) {
                                               assert(value == state.player && !state.player);
                                               stores.push_back(2);
                                           },
                                           [&](std::uint8_t value) {
                                               assert(value == state.super_sonic);
                                               stores.push_back(3 + value);
                                           }};
    state.primary = first;
    actor_current::set_primary(state, second, &observation);
    actor_current::set_primary(state, second, &observation); // same-value publication
    actor_current::set_player(state, {}, &observation);      // authoritative null
    actor_current::set_super_sonic(state, 1, &observation);
    actor_current::set_super_sonic(state, 0, &observation);
    assert((stores == std::vector<int>{1, 1, 2, 4, 3}));
    assert(actor_current::read_super_sonic(state) == 0);
    actor_current::Observation empty{};
    actor_current::set_primary(state, first, &empty);
    actor_current::set_player(state, second, &empty);
    actor_current::set_super_sonic(state, 173, &empty);
    assert(state.primary == first && state.player == second && state.super_sonic == 173);
    observation.super_sonic = [](std::uint8_t) { throw std::runtime_error("receipt"); };
    try {
        actor_current::set_super_sonic(state, 0, &observation);
        assert(false);
    } catch (const std::runtime_error &) {
        assert(state.super_sonic == 0); // failed observation cannot veto the store
    }

    // Holdout: independently allocated worlds never share the current slots.
    for (unsigned i = 0; i < 257; ++i) {
        actor_current::State other;
        actor_current::set_primary(other, second);
        actor_current::set_player(other, first);
        actor_current::set_super_sonic(other, static_cast<std::uint8_t>(i));
        assert(other.primary == second && other.player == first);
        assert(other.super_sonic == static_cast<std::uint8_t>(i));
        assert(state.primary == first && state.player == second && state.super_sonic == 0);
    }
}
