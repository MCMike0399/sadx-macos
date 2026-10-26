#include "guest/polybuff_owners.hpp"
#include "engine/ninja_device_init.hpp"
#include "engine/ninja_system_init.hpp"
#include <cassert>
#include <cstdio>

int polybuff_native_device_unit(int argc, char **argv);

int main(int argc, char **argv) {
    using namespace sadx;
    NinjaFrameCallState frame;
    NinjaDeviceState device;
    NinjaSystemInitState system;
    NinjaMemoryState memory;
    GuestHeap heap, crt(0x520000, 0x60000000);
    NinjaTexturePoolState textures;
    NinjaTextureSelectionState selection;
    NinjaCallbackList callbacks;
    std::optional<std::uint32_t> queue, window, instance;
    std::array<std::optional<std::uint32_t>, 4> entries{}, clear{};
    std::array<std::optional<std::uint32_t>, 9> counts{};
    for (unsigned i = 0; i < 9; ++i) {
        counts[i] = i + 1;
        system.tables_at_389d888[i] = 36;
        system.tables_at_389d888[9+i] = 338;
        system.tables_at_389d888[18+i] = 0x700000 + i;
    }
    device.polygon_scratch.words.fill(0x44444444);
    unsigned calls = 0;
    X87Context fp(127);
    NinjaSystemInitRuntime runtime{system, {memory, heap, crt, frame,
        *device.polygon_scratch_facts, counts, textures, selection, callbacks, queue, entries,
        &clear, window, instance}, fp, {}, {}};
    runtime.invoke = [&](const NinjaSystemCall &call) -> NinjaSystemOwnerResult {
        assert(call.callee == 0x794540 && *call.ecx == 0x3d0fe00 + 36*calls);
        assert(*call.edx == calls + 1);
        ++calls;
        return {{GameLogicOwnerState::completed, 0, {}}, {}};
    };
    // The native startup hooks in this executable throw. Only correct ADL reference
    // dispatch can complete all nine calls and the actual shared scratch clear.
    assert(initialize_system_polygon_buffers(runtime, 0x6000, 0x38fcf08));
    assert(calls == 9 && crt.live_blocks() == 1);
    assert(device.polygon_scratch.words.front() == 0 && device.polygon_scratch.words.back() == 0);
    const auto native_result = polybuff_native_device_unit(argc, argv);
    assert(native_result == 0);
    std::puts("One process: nine reference initializer calls and Root actual native device unit pass");
    return 0;
}
