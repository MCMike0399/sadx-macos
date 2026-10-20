#include "engine/task_work.hpp"
#include "hle/hle_process.hpp"
#include "hle/hle_task_bridge.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
using namespace sadx;

void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}

struct Fixture {
    std::unique_ptr<HleProcess> process;

    Fixture(const ExecutableImage& image, bool native) {
        std::string error;
        process = HleProcess::create(".", "/private/tmp/sadx-inactive-lifetime-overlay", &error);
        require(bool(process), error);
        require(process->map_main(image, &error), error);
        require(process->guest().add_override(0x51a740, [](X86Guest&) { return 0U; }, &error),
                error);
        if (native)
            for (const auto& entry : task_overrides(*process))
                require(process->guest().add_override(entry.address, entry.bridge, &error), error);
        call(0x40b1a0, {});
        call(0x40b460, {});
    }

    std::uint32_t call(std::uint32_t entry, std::vector<std::uint32_t> arguments) {
        const auto result = process->guest().call(entry, arguments);
        require(bool(result), result.error);
        return result.eax;
    }

    std::vector<std::uint8_t> arena() {
        const auto base = process->guest().u32(0x3abdbf0).value_or(0);
        std::vector<std::uint8_t> bytes(TaskWorkArena::usable_bytes + 32);
        require(base && process->guest().read(base, bytes.data(), bytes.size()), "arena readable");
        return bytes;
    }
};

void compare(Fixture& original, Fixture& native) {
    require(original.arena() == native.arena(), "full original/native arena equality");
    for (const auto address : {0x3abdbc4U, 0x3abdbc8U, 0x3abdbccU, 0x3abdbd0U,
                               0x3abdbd4U, 0x3abdbd8U, 0x3abdbdcU, 0x3abdbe0U,
                               0x3abdbe8U, 0x3abdbecU, 0x3abdbf4U, 0x3abdbf8U})
        require(original.process->guest().u32(address) == native.process->guest().u32(address),
                "original/native task globals equality");
}

void inactive(Fixture& original, Fixture& native, std::uint32_t task) {
    const auto before = original.arena();
    require(original.call(0x40b570, {task}) == 0, "source rejects inactive destruction");
    require(original.arena() == before, "source inactive destruction has no arena stores");
    require(native.call(0x40b570, {task}) == 0, "native rejects inactive destruction");
    compare(original, native);
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "usage: hle-task-inactive-lifetime-test SONIC.EXE");
        std::string error;
        std::ifstream file(argv[1], std::ios::binary);
        const auto image = ExecutableImage::parse({std::istreambuf_iterator<char>(file), {}}, &error);
        require(bool(image), error);
        Fixture original(*image, false), native(*image, true);
        compare(original, native);
        // Chaos 0 death cleanup retains a pointer after another owner destroyed its
        // task. DestroyTask first calls CheckTask, so a known free pool row is a no-op.
        const auto free = original.process->guest().u32(0x3abdbec).value_or(0);
        require(free != 0 && native.process->guest().u32(0x3abdbec) == free, "same free row");
        inactive(original, native, 0);
        inactive(original, native, free);
        const auto parent = original.call(0x40b860, {2, 0, 0x51f680});
        require(parent == native.call(0x40b860, {2, 0, 0x51f680}), "same parent identity");
        const auto child = original.call(0x40b940, {2, 0x51f680, parent});
        require(child == native.call(0x40b940, {2, 0x51f680, parent}), "same child identity");
        original.call(0x40b570, {parent});
        native.call(0x40b570, {parent});
        compare(original, native);
        inactive(original, native, parent);
        inactive(original, native, child);
        const auto detached = original.call(0x40b860, {0, 7, 0x51f680});
        require(detached == native.call(0x40b860, {0, 7, 0x51f680}), "same detached identity");
        inactive(original, native, detached);
        const auto foreign = native.process->guest().call(0x40b570, {0x12345678});
        require(!foreign && foreign.error.find("non-pool task") != std::string::npos,
                "foreign task identity remains rejected");
        std::cout << "Task lifetime: never-allocated, destroyed parent, recursively destroyed child "
                     "and detached task reject destruction without stores; original/native full "
                     "arena bytes and task globals match; null accepted and foreign identity rejected\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
