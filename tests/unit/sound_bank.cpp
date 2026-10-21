#include "game/sound_bank.hpp"
#include <cassert>
#include <vector>
using namespace sadx::sound_bank;
int main() {
    Catalog catalog;
    catalog[42] = {{0, "FIRST"}, {1, "SECOND"}, {5, ""}};
    State state;
    std::uint32_t initialized = 0;
    std::vector<std::string> calls;
    Services services{[&](unsigned slot) {
                          calls.push_back("release" + std::to_string(slot));
                      },
                      [&](const Handle &bank) {
                          calls.push_back("close:" + (bank ? bank->loaded_path() : "null"));
                      },
                      [&](std::string_view path) {
                          calls.emplace_back(path);
                          return -1;
                      },
                      [&](std::string_view path, unsigned) -> Handle {
                          calls.emplace_back(path);
                          return nullptr;
                      },
                      [&](std::string_view path, unsigned) -> Handle {
                          calls.emplace_back(path);
                          return path.find("\\.dat") == std::string_view::npos
                                     ? std::make_shared<Bank>(std::string(path))
                                     : nullptr;
                      }};
    select(42, catalog, state, initialized, true, "D:\\", services);
    assert(calls.empty());
    initialized = 1;
    select(42, catalog, state, initialized, false, "D:\\", services);
    select(123, catalog, state, initialized, true, "D:\\", services);
    assert(calls.empty());
    select(42, catalog, state, initialized, true, "D:\\", services);
    assert(state.slots[0] && state.slots[1] && !state.slots[5] && calls.size() == 13);
    calls.clear();
    const auto first = state.slots[0];
    state.slots[1] = std::make_shared<Bank>("system\\sounddata\\se\\second\\");
    select(42, catalog, state, initialized, true, "D:\\", services);
    assert(state.slots[0] == first && calls.size() == 5);
    catalog[42].resize(1);
    state.slots[0] = std::make_shared<Bank>("old.dat");
    services.release_samples = [&](unsigned slot) {
        state.slots[slot] = std::make_shared<Bank>("replacement.dat");
    };
    services.close = [&](const Handle &bank) {
        assert(bank->loaded_path() == "replacement.dat");
        state.slots[0] = std::make_shared<Bank>("close replacement.dat");
        assert(bank->loaded_path() == "replacement.dat");
    };
    services.open_file = [&](std::string_view path, unsigned) -> Handle {
        assert(!state.slots[0]);
        return std::make_shared<Bank>(std::string(path));
    };
    select(42, catalog, state, initialized, true, "", services);
    assert(state.slots[0]->loaded_path() == "SYSTEM\\SoundData\\SE\\FIRST.dat");
    bool unsupported = false;
    try {
        select(-1, catalog, state, initialized, true, "", services);
    } catch (const Error &error) {
        unsupported = error.boundary() == Boundary::ListIndex;
    }
    assert(unsupported);
    std::int32_t counter = 0;
    assert(!normalize_name("_B02_00_15.WAV", 2, counter, false) && counter == 0);
    assert(normalize_name("B00_00_25.WAV", 0, counter, false) == "B00_00_25.WAV" && counter == 1);
    assert(normalize_name("other.wav", 4, counter, true) == "B04_00_01.ADX" && counter == 2);
}
