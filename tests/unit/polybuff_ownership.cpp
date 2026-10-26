#include "guest/polybuff_owners.hpp"
#include "guest/frame_control_reference.hpp"
#include "engine/ninja_device_init.hpp"
#include "engine/ninja_system_init.hpp"
#include <cstdio>
#include <cstdlib>

namespace {
unsigned checks = 0;
void require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(1);
    }
    ++checks;
}
}
int main() {
    using namespace sadx;
    NinjaFrameCallState frame;
    NinjaDeviceState device;
    NinjaSystemInitState system;
    auto &facts = *frame.polygon_facts;
    for (unsigned i = 0; i < 9; ++i) {
        const auto &record = frame.polygon_buffers.records[i];
        require(record.resource == nullptr && record.capacity_bytes == 0 &&
                record.remaining_bytes == 0 && record.stride == 0 && record.vertex_format == 0 &&
                record.render_arguments.words.data == nullptr && record.lock_count == 0 &&
                record.debug_name == nullptr && record.unused == 0, "canonical BSS records");
        facts.records[i][1] = 100 + i;
        require(record.capacity_bytes == 100 + i, "reference prefix writes canonical capacity");
        frame.polygon_buffers.records[i].capacity_bytes = 200 + i;
        require(facts.records[i][1] == 200 + i, "native writes visible through reference view");
        facts.records[i][4] = 338;
        facts.records[i][5] = 0x20000000;
        facts.records[i][6] = 7;
        facts.records[i][7] = 0x88aea4;
        facts.records[i][8] = 123;
        require(record.vertex_format == 338 && record.lock_count == 7 && record.unused == 123,
                "all nine words share the complete record owner");
    }
    require(!device.polygon_scratch_facts->isolated &&
            &device.polygon_scratch_facts->owner == &device.polygon_scratch,
            "device scratch adapter borrows the actual owner");
    device.polygon_scratch.words[4] = 0x12345678;
    device.polygon_scratch_facts->bytes[16] = 0xab;
    require(device.polygon_scratch.words[4] == 0x123456ab, "byte stores preserve other scratch lanes");
    device.polygon_scratch_facts->bytes[17].available = true;
    require(device.polygon_scratch_facts->bytes[17].value_or(0) == 0x56,
            "scratch byte read observes actual canonical word");
    facts.vertices = 0x60000000;
    polybuff_reference::VertexScratchRead read(facts.vertices);
    require(*read == 0x60000000, "polygon consumer borrows live canonical identity");
    frame.polygon_buffers.vertices = reinterpret_cast<polybuff::BufferMemory *>(0x60001000);
    require(*read == 0x60001000, "polygon consumer rereads after native mutation");
    facts.vertices = std::nullopt;
    require(!read && frame.polygon_buffers.vertices != nullptr,
            "reference availability is separate from retained game value");
    *system.polygon_base_fact = 0x38fcf08;
    require(system.polygon_base.value == reinterpret_cast<void *>(0x38fcf08),
            "retained base has one pointer owner");

    NinjaMemoryState memory;
    GuestHeap heap, crt(0x520000, 0x60000000);
    NinjaTexturePoolState textures;
    NinjaTextureSelectionState selection;
    NinjaCallbackList callbacks;
    std::optional<std::uint32_t> queue, window, instance;
    std::array<std::optional<std::uint32_t>, 4> entries{}, clear{};
    std::array<std::optional<std::uint32_t>, 9> counts{};
    for (unsigned i = 0; i < 9; ++i) counts[i] = i + 1;
    for (unsigned i = 0; i < 27; ++i) system.tables_at_389d888[i] = 100 + i;
    device.polygon_scratch.words.fill(0x44444444);
    X87Context fp(127);
    unsigned calls = 0;
    NinjaSystemInitRuntime runtime{system, {memory, heap, crt, frame,
        *device.polygon_scratch_facts, counts, textures, selection, callbacks, queue, entries,
        &clear, window, instance}, fp, {}, {}};
    runtime.invoke = [&](const NinjaSystemCall &call) -> NinjaSystemOwnerResult {
        const auto index = calls++;
        require(call.callee == 0x794540 && *call.ecx == 0x3d0fe00 + 36 * index &&
                *call.edx == *counts[index], "shared core reaches each actual record and live count");
        if (index == 0) counts[8] = 32000;
        // The real reference owner can mutate canonical state during its callback.
        frame.polygon_buffers.records[index].stride = *call.arguments[0];
        return {{GameLogicOwnerState::completed, 0, {}}, {}};
    };
    require(bool(initialize_system_polygon_buffers(runtime, 0x6000, 0x38fcf08)) && calls == 9,
            "legacy adapter executes the full shared nine-record initializer");
    require(frame.polygon_buffers.records[8].stride == 108 &&
            device.polygon_scratch.words.back() == 0 && crt.live_blocks() == 1,
            "shared core retains callback mutations, clears full scratch and owns allocation lifetime");
    require(crt.write(0x60000000 + 0x6000 * 36 - 1, 0xab) &&
            crt.read(0x60000000 + 0x6000 * 36 - 1) == 0xab,
            "CPU scratch allocation has actual backing bytes throughout owner lifetime");
    counts[3].reset();
    device.polygon_scratch.words.fill(0x55555555);
    calls = 0;
    const auto failed = initialize_system_polygon_buffers(runtime, 1, 42);
    require(!failed && failed.source_boundary == 0x78e763 &&
            failed.required_fact == 0x389d864 + 12 && calls == 3 &&
            device.polygon_scratch.words.front() == 0x55555555 &&
            system.polygon_base.value == reinterpret_cast<void *>(42),
            "missing count stops at reached read after exact allocation/call prefix");
    GameFrameRendererState renderer{};
    renderer.counter_frequency.low = 0;
    renderer.counter_frequency.high = 0;
    NinjaRenderState render;
    NinjaDeviceInitRuntime device_runtime{device, frame, render, renderer, fp, {}, {}};
    device_runtime.host.create_direct3d = [&](std::uint32_t sdk) -> std::optional<std::uint32_t> {
        require(sdk == 220, "actual Direct3D SDK operand reaches the host boundary");
        return {};
    };
    const auto stopped = run_device_setup_791610(device_runtime);
    require(!stopped && stopped.source_boundary == 0x791651,
            "device setup stops explicitly at unavailable host after source clears");
    for (unsigned i = 0; i < 9; ++i)
        require(frame.polygon_buffers.records[i].resource == nullptr &&
                frame.polygon_buffers.records[i].vertex_format == 338 &&
                frame.polygon_buffers.records[i].lock_count == 7,
                "device setup clears only each resource in the same complete record owner");
    require(device.polygon_scratch.words.front() == 0 && device.polygon_scratch.words.back() == 0,
            "device setup clears the actual shared scratch owner");
    std::printf("%u ownership and shared-core checks pass\n", checks);
}
