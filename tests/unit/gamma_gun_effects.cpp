#include "player/gamma_gun_effects.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool value, const std::string &message) {
    if (!value)
        throw std::runtime_error(message);
}

struct Fake final : sadx::gamma_gun_effects::Services {
    std::uintptr_t selected = 0;
    int pass = -1;
    std::uint32_t word = 0x20;
    std::vector<std::string> calls;
    std::uintptr_t select_lock(int requested) override {
        pass = requested;
        calls.push_back("select");
        return selected;
    }
    std::uint8_t read_variant(std::uintptr_t) override {
        calls.push_back("variant");
        return 3;
    }
    std::uint32_t variant_word(std::uint8_t) override {
        calls.push_back("word");
        return word;
    }
    void open_volley(sadx::gamma_gun_effects::GunState &, std::uintptr_t) override {
        calls.push_back("volley");
    }
};

sadx::gamma_gun_effects::GunState fresh(std::int8_t mode, float charge = 0) {
    sadx::gamma_gun_effects::GunState state;
    state.laser_mode = 1;
    state.mode = mode;
    state.marker = 0x5a;
    state.lock_count = 4;
    state.armed = 9;
    state.variant = 9;
    state.charge = charge;
    state.variant_value = 0x111;
    state.fade = 4.0f;
    return state;
}

void run(sadx::gamma_gun_effects::GunState &state, Fake &services) {
    sadx::fidelity::Context arithmetic(0x007f);
    sadx::gamma_gun_effects::update(state, services, arithmetic);
}
} // namespace

int main() {
    {
        Fake services;
        auto state = fresh(0);
        state.lock_count = 0;
        run(state, services);
        require(services.calls.size() == 1 && services.calls[0] == "word", "an empty list only reads the variant table");
        require(state.mode == 1 && state.fade == 0.0f, "mode 0 enters the idle frame");
        require(state.marker == 0 && state.variant == 0, "the idle frame clears the marker and variant");
        require(state.variant_value == services.word, "the idle frame reads table entry 0");
        require(state.lock_count == 0 && state.armed == 0, "every return clears the lock count");
    }
    {
        Fake services;
        auto state = fresh(6, 3.0f);
        run(state, services);
        require(services.calls.empty() && state.mode == 6 && state.fade == 4.0f, "mode 6 only finishes");
        require(state.marker == 0x5a && state.variant == 9, "mode 6 leaves the marker and variant");
        require(state.lock_count == 0 && state.armed == 0, "mode 6 still clears the lock count");
    }
    {
        Fake services;
        auto state = fresh(4, 13.0f);
        state.lock_count = 0;
        run(state, services);
        require(state.mode == 4 && state.charge == 14.0f, "charge below 15 stays in mode 4");
        require(state.marker == 0x5a, "a short charge does not ask the lock list");
        require(state.armed == 0 && state.lock_count == 0, "a short charge still finishes");
    }
    {
        Fake services;
        auto state = fresh(4, 14.0f);
        state.lock_count = 0;
        run(state, services);
        require(state.mode == 5 && state.charge == 15.0f, "charge 15 opens mode 5");
        require(state.marker == 0, "mode 5 asks the empty lock list");
        require(services.calls.empty(), "a null selection does not open a volley");
    }
    {
        Fake services;
        auto state = fresh(5, 29.0f);
        state.lock_count = 0;
        run(state, services);
        require(state.mode == 3 && state.charge == 0.0f, "charge 30 returns to mode 3");
        require(state.marker == 0 && state.armed == 0, "the hot path still finishes");
    }
    {
        Fake services;
        auto state = fresh(5, 0.0f);
        state.lock_count = 0;
        run(state, services);
        require(state.mode == 5, "charge 1 stays in mode 5");
        require(state.fade != 4.0f, "mode 5 writes the fade");
    }
    {
        Fake services;
        auto state = fresh(3);
        state.lock_count = 0;
        state.variant_value = 0x222;
        run(state, services);
        require(state.mode == 0 && state.fade == 0.0f, "a null mode 3 returns to mode 0");
        require(state.variant == 9 && state.variant_value == 0x222, "a null mode 3 does not read the table");
    }
    {
        Fake services;
        services.selected = 8;
        auto state = fresh(1);
        state.lock_count = 1;
        run(state, services);
        require(services.pass == 0 && state.mode == 2 && state.charge == 14.0f, "a lock starts the volley");
        require(!services.calls.empty() && services.calls.back() == "volley", "the volley is the last call");
    }
    std::cout << "Gamma gun effects: idle frame, charge gates and empty lock list passed\n";
    return 0;
}
