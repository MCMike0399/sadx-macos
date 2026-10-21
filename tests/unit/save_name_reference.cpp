#include "guest/save_name_reference.hpp"
#include "guest/game_mode_leaves_reference.hpp"
#include <array>
#include <cassert>
#include <cstring>
#include <memory>
#include <optional>
#include <utility>
namespace {
struct Leaves {
    sadx::GameModeState mode;
    sadx::GameLogicTimingState timing;
    std::optional<std::int32_t> cutscene;
    std::optional<sadx::TaskHandle> event;
    sadx::CrtRandState random;
    std::optional<std::uint16_t> game_mode, stage;
    sadx::GameModeLeafState state{mode, timing, cutscene, event, random, game_mode, stage};
};
} // namespace
int main() {
    using sadx::SaveNameReference;
    using sadx::save::SaveNameState;
    SaveNameState native;
    assert(native.value == nullptr);
    // Native strings are ordinary resources; the shared owner publishes only one pointer.
    auto backing = std::make_unique<char[]>(14);
    std::memcpy(backing.get(), "SonicDX01.snc", 14);
    native.value = backing.get();
    const char *&borrow = native.value;
    assert(borrow == backing.get());
    backing.reset();
    borrow = nullptr;
    assert(native.value == nullptr);
    SaveNameReference transport;
    SaveNameState observed;
    for (auto raw : std::array<std::uint32_t, 7>{0, 1, 0x1000, 0x80000000, 0xdeadbeef, 0xfffffffe,
                                                 0xffffffff}) {
        transport.import_value(observed, raw);
        assert(transport.export_value(observed) == raw);
        if (raw != 0) {
            assert(observed.value != nullptr);
            const auto identity = observed.value;
            transport.import_value(observed, raw);
            assert(observed.value == identity);
        }
    }
    transport.import_value(observed, std::nullopt);
    assert(observed.value != nullptr && !transport.export_value(observed));
    const auto unknown = observed.value;
    auto snapshot = std::make_pair(observed, transport);
    transport = {};
    observed.value = nullptr;
    assert(snapshot.first.value == unknown && !snapshot.second.export_value(snapshot.first));
    snapshot.first.value = nullptr;
    assert(snapshot.second.export_value(snapshot.first) == 0);
    transport.import_value(observed, 0x12345678);
    auto known_snapshot = std::make_pair(observed, transport);
    transport = {};
    observed.value = nullptr;
    assert(known_snapshot.second.export_value(known_snapshot.first) == 0x12345678);
    const char real_native[] = "SonicDX01.snc";
    observed.value = real_native;
    assert(!transport.export_value(observed)); // Never truncate/guess a native pointer value.
    for (auto raw : {std::optional<std::uint32_t>{}, std::optional<std::uint32_t>{0},
                     std::optional<std::uint32_t>{0xdeadbeef}}) {
        Leaves fixture;
        auto &state = fixture.state;
        state.save_name_reference.import_value(state.save_name, raw);
        const auto saved = state.save_name.value;
        const auto unchanged = sadx::run_game_mode_leaf(0x421f70, {}, state, {});
        assert(unchanged.outcome == sadx::GameModeLeafOutcome::completed);
        assert(state.save_name.value == saved &&
               state.save_name_reference.export_value(state.save_name) == raw);
        auto copy = state;
        const auto reset = sadx::run_game_mode_leaf(0x421f90, {}, state, {});
        assert(reset.outcome == sadx::GameModeLeafOutcome::completed &&
               state.save_name.value == nullptr);
        assert(state.save_name_reference.export_value(state.save_name) == 0);
        assert(copy.save_name.value == saved &&
               copy.save_name_reference.export_value(copy.save_name) == raw);
    }
    // The reference default is unknown, not an assumed sourceBSS value.
    Leaves untouched;
    assert(untouched.state.save_name.value != nullptr);
    assert(!untouched.state.save_name_reference.export_value(untouched.state.save_name));
}
