#include "hle/hle_process.hpp"
#include "hle/hle_task_bridge.hpp"
#include <array>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {
using namespace sadx;
struct Fixture {
    std::unique_ptr<HleProcess> process;
    Fixture(bool globals = true) {
        std::string error;
        process = HleProcess::create(".", "build/task-judges/bridge-overlay", &error);
        assert(process);
        auto& g = process->guest();
        for (auto base : {0x7db000U, 0x40b000U, 0x64f000U, 0x51a000U})
            assert(g.map(base, 4096, true, &error));
        if (globals) {
            assert(g.map(0x3abd000, 4096, true, &error));
            assert(g.map(0x38f6000, 4096, true, &error));
        }
        const std::uint8_t ret = 0xc3;
        assert(g.write(0x64fc70, &ret, 1) && g.write(0x51a740, &ret, 1));
        auto create = process->hle().trap(g, "kernel32.dll", "HeapCreate", &error);
        auto allocate = process->hle().trap(g, "kernel32.dll", "HeapAlloc", &error);
        assert(create && allocate);
        assert(g.set_u32(0x7db078, *create) && g.set_u32(0x7db074, *allocate));
        for (const auto& entry : task_overrides(*process))
            assert(g.add_override(entry.address, entry.bridge, &error));
    }
    GuestCallResult call(unsigned entry, std::initializer_list<unsigned> args) {
        return process->guest().call(entry, args);
    }
    void arena() { assert(call(0x40b1a0, {})); }
    void pool() { arena(); assert(call(0x40b460, {})); }
};
void missing_and_lazy() {
    Fixture missing(false);
    auto& g = missing.process->guest();
    // Invalid index exits before unavailable initialized-arena/pool metadata.
    auto lazy = g.call(0x40b030, {}, {}, 9);
    assert(lazy && lazy.eax == 0);
    const auto failure = missing.call(0x40b1a0, {});
    assert(!failure && failure.error.find("missing arena global at 40b1a5") != std::string::npos);
    std::cout << "lazy-invalid-level " << lazy.eax << '\n' << failure.error << '\n';
    Fixture empty;
    // A real zero free head returns allocation failure before arena initialization.
    const auto zero = empty.call(0x40b860, {15, 0, 0});
    assert(zero && zero.eax == 0);
    empty.arena();
    std::string error;
    assert(empty.process->guest().unmap(0x38f6000, 4096, &error));
    const auto absent = empty.call(0x40b460, {});
    assert(!absent && absent.error.find("missing field 38f6e08 at 40b460") != std::string::npos);
    assert(empty.process->guest().u32(0x3abdbe4) == 0);
    std::cout << "lazy-zero-free " << zero.eax << '\n' << absent.error << '\n';
}
void pool_failures() {
    Fixture unreadable;
    unreadable.pool();
    auto& g = unreadable.process->guest();
    const auto pool = *g.u32(0x3abdbe4);
    const auto free = g.u32(0x3abdbec);
    const auto head = g.u32(0x3abdbc4);
    std::string error;
    assert(g.unmap((pool & ~4095U) + 4096, 4096, &error));
    auto result = unreadable.call(0x40b860, {0, 0, 0});
    assert(!result && result.error.find("unreadable pool at 40b860") != std::string::npos);
    assert(g.u32(0x3abdbec) == free && g.u32(0x3abdbc4) == head);
    std::cout << result.error << " heads-and-free-unchanged\n";

    Fixture partial;
    partial.arena();
    auto& writer = partial.process->guest();
    const auto base = *writer.u32(0x3abdbf0);
    // Leave allocator header/end metadata available; remove an interior pool page.
    assert(writer.unmap((base & ~4095U) + 4096, 4096, &error));
    result = partial.call(0x40b460, {});
    assert(!result && result.error.find("pool native boundary 40b4bd") != std::string::npos);
    assert(writer.u32(0x3abdbe4) == 0 && writer.u32(0x3abdbec) == 0);
    std::vector<std::uint8_t> prefix(((base & ~4095U) + 4096) - base);
    assert(writer.read(base, prefix.data(), prefix.size()));
    unsigned zero = 0, ff = 0;
    for (auto byte : prefix) { zero += byte == 0; ff += byte == 255; }
    // Full byte prefix is emitted, including actual failed transport's partial stores.
    std::cout << result.error << " prefix " << zero << ' ' << ff << '\n';
    constexpr char digits[] = "0123456789abcdef";
    for (auto byte : prefix) std::cout << digits[byte >> 4] << digits[byte & 15];
    std::cout << '\n';
}
} // namespace
int main() { missing_and_lazy(); pool_failures(); }
