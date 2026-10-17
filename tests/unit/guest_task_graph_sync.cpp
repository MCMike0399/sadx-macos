#include "hle/hle_process.hpp"
#include "hle/hle_task_bridge.hpp"
#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>
#include <unicorn/unicorn.h>
namespace {
void require(bool value, const std::string &why) {
    if (!value)
        throw std::runtime_error(why);
}
} // namespace
int main() {
    try {
        std::string error;
        auto process = sadx::HleProcess::create(".", "build/task-sync-overlay", &error);
        require(bool(process), error);
        auto &guest = process->guest();
        for (auto base :
             {0x3abd000U, 0x38f6000U, 0x7db000U, 0x40b000U, 0x64f000U, 0x51a000U, 0x10000000U})
            require(guest.map(base, 4096, true, &error), error);
        // Isolate graph synchronization: unrelated retained CRT and initialization
        // prerequisites are explicit RET fixtures. The full task-bridge suite covers retained CRT and real heap acquisition.
        const std::uint8_t prerequisite_ret = 0xc3;
        require(guest.write(0x64fc70, &prerequisite_ret, 1) &&
                    guest.write(0x51a740, &prerequisite_ret, 1),
                "prerequisite fixtures");
        const auto create = process->hle().trap(guest, "kernel32.dll", "HeapCreate", &error);
        const auto allocate = process->hle().trap(guest, "kernel32.dll", "HeapAlloc", &error);
        require(create && allocate, error);
        guest.set_u32(0x7db078, *create);
        guest.set_u32(0x7db074, *allocate);
        for (auto entry : sadx::task_overrides(*process))
            require(guest.add_override(entry.address, entry.bridge, &error), error);
        const auto call = [&](std::uint32_t target, std::initializer_list<std::uint32_t> args) {
            auto result = guest.call(target, args);
            require(bool(result), result.error + " target " + std::to_string(target) + " pc " +
                                      std::to_string(guest.reg(UC_X86_REG_EIP)));
            return result.eax;
        };
        call(0x40b1a0, {});
        call(0x40b460, {});
        constexpr unsigned count = 128;
        for (unsigned n = 0; n < count; ++n)
            require(call(0x40b860, {0, 0, 0x10000000}) != 0, "task allocation");
        std::vector<std::uint32_t> expected;
        for (auto task = guest.u32(0x3abdbc4).value_or(0); task;
             task = guest.u32(task).value_or(0)) {
            require(expected.size() < count, "acyclic list");
            expected.push_back(task);
        }
        require(expected.size() == count, "all callbacks linked");
        const auto free_slot = guest.u32(0x3abdbec).value_or(0);
        require(free_slot != 0, "free slot");
        const std::uint8_t ret = 0xc3;
        require(guest.write(0x10000000, &ret, 1), "callback code");
        std::vector<std::uint32_t> observed;
        require(guest.add_instruction_hook(
                    0x10000000,
                    [&](sadx::X86Guest &g, std::uint32_t) {
                        const auto task = g.u32(g.esp() + 4).value_or(0);
                        observed.push_back(task);
                        g.set_u32(task + 48, 0x30000000U + static_cast<unsigned>(observed.size()));
                        g.set_u32(free_slot + 48,
                                  0xabc00000U + static_cast<unsigned>(observed.size()));
                        return false;
                    },
                    &error),
                error);
        for (unsigned run = 0; run < 2; ++run) {
            observed.clear();
            const auto before = guest.memory_reads();
            call(0x40b0c0, {0});
            const auto after = guest.memory_reads();
            require(observed == expected, "callback order, exactly once per task");
            for (unsigned n = 0; n < count; ++n)
                require(guest.u32(expected[n] + 48) == 0x30000000U + n + 1,
                        "callback-owned resource mutation preserved");
            require(guest.u32(free_slot + 48) == 0xabc00000U + count,
                    "opaque free-slot callback store preserved");
            const auto calls = after.calls - before.calls;
            std::cout << "Task synchronization: " << count << " callbacks, " << calls
                      << " API reads, " << after.bytes - before.bytes << " bytes\n";
            // A per-slot/per-word API sweep at every callback is the measured bug.
            // Permit page/block batching and future implementations, not one exact count.
            require(calls <= count * 256 + 4096,
                    "task graph synchronization still performs scalar pool sweeps");
        }
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
