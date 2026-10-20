#include "engine/task_work.hpp"
#include "hle/hle_process.hpp"
#include "hle/hle_task_bridge.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <unicorn/unicorn.h>
using namespace sadx;
void require(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}
struct Fixture {
  std::unique_ptr<HleProcess> process;
  std::uint32_t input = 0;
  Fixture(const std::string &game, const std::string &system, bool native,
          const std::string &overlay) {
    std::string error;
    std::ifstream file(game + "/sonic.exe", std::ios::binary);
    const auto image = ExecutableImage::parse(
        {std::istreambuf_iterator<char>(file), {}}, &error);
    require(bool(image), error);
    process = HleProcess::create(game, overlay, &error);
    require(bool(process), error);
    process->use_real_cpp_runtime(system);
    require(process->map_main(*image, &error), error);
    require(bool(process->guest().call(0x401ac0, {}, 0x3ab9170)),
            "mod startup");
    input = process->modules().by_name("sadx-input-mod.dll")->base;
    if (native)
      for (const auto &entry : task_overrides(*process))
        require(
            process->guest().add_override(entry.address, entry.bridge, &error),
            error);
    require(process->guest().add_override(
                0x51a740, [](X86Guest &) { return 0U; }, &error),
            error);
    call(0x40b1a0, {});
    call(0x40b460, {});
  }
  X86Guest &guest() { return process->guest(); }
  GuestCallResult call(std::uint32_t at, std::vector<std::uint32_t> args) {
    auto result = guest().call(at, args);
    require(bool(result), result.error);
    return result;
  }
  std::uint32_t create() {
    // The pinned optimized internal function receives port in ECX, motor in
    // EDX, and time on the stack (source Rumble_Load_hook; input DLL RVA
    // 0x8f60).
    guest().set_reg(UC_X86_REG_ECX, 0);
    guest().set_reg(UC_X86_REG_EDX, 1);
    call(input + 0x8f60, {1});
    const auto task = guest().u32(0x3abdbc4).value_or(0);
    require(task != 0, "rumble task created");
    return task;
  }
};
void same_arena(Fixture &original, Fixture &native) {
  const auto base = original.guest().u32(0x3abdbf0).value_or(0);
  require(base == native.guest().u32(0x3abdbf0).value_or(0),
          "same arena identity");
  std::vector<std::uint8_t> a(TaskWorkArena::usable_bytes + 32), b(a.size());
  require(original.guest().read(base, a.data(), a.size()) &&
              native.guest().read(base, b.data(), b.size()),
          "arena bytes readable");
  require(a == b, "original/native arena and task pool bytes");
  require(original.process->memory().live_blocks() ==
              native.process->memory().live_blocks(),
          "original/native CRT/heap lifetime");
}
int main(int argc, char **argv) {
  try {
    require(argc == 3,
            "usage: hle-task-rumble-test GAME_DIR WINDOWS_SYSTEM_DIR");
    Fixture original(argv[1], argv[2], false,
                     "/private/tmp/sadx-rumble-original-overlay");
    Fixture native(argv[1], argv[2], true,
                   "/private/tmp/sadx-rumble-native-overlay");
    const auto task = original.create();
    require(task == native.create(), "same task identity");
    const auto timer = original.guest().u32(task + 40).value_or(0);
    require(timer && native.guest().u32(task + 40) == timer,
            "same CRT timer pointer");
    require(original.process->memory().size(timer).value_or(0) >= 8,
            "timer is live CRT allocation");
    same_arena(original, native);
    for (unsigned frame = 0; frame < 6; ++frame) {
      original.call(0x40b170, {});
      native.call(0x40b170, {});
      same_arena(original, native);
    }
    require(!original.process->memory().size(timer) &&
                !native.process->memory().size(timer),
            "actual mod destructor freed CRT timer");
    require(native.guest().u32(task + 40) == 0,
            "actual mod destructor cleared UnknownA_ptr");
    const auto a = original.call(0x40b860, {4, 0, 0x51f680}).eax,
               b = native.call(0x40b860, {4, 0, 0x51f680}).eax;
    require(a == task && b == task, "reused task slot");
    original.call(0x40b170, {});
    native.call(0x40b170, {});
    same_arena(original, native);
    original.call(0x40b570, {a});
    native.call(0x40b570, {b});
    same_arena(original, native);
    original.create();
    native.create();
    original.call(0x40b460, {});
    native.call(0x40b460, {});
    const auto reset_original = original.call(0x40b860, {0, 0, 0x51f680}).eax;
    const auto reset_native = native.call(0x40b860, {0, 0, 0x51f680}).eax;
    require(reset_original == reset_native &&
                native.guest().u32(reset_native + 40) == 0,
            "reinitialized pool does not inherit rumble payload by slot");
    original.call(0x40b170, {});
    native.call(0x40b170, {});
    same_arena(original, native);
    unsigned negatives = 0;
    for (unsigned scenario = 0; scenario < 3; ++scenario) {
      Fixture f(argv[1], argv[2], true,
                "/private/tmp/sadx-rumble-negative-" +
                    std::to_string(scenario));
      const auto t = f.create();
      GuestCallResult result;
      if (scenario == 0) {
        f.guest().set_u32(t + 24, 0x51f680);
        result = f.guest().call(0x40b170, {});
        require(!result && result.error.find("unowned work field") !=
                               std::string::npos,
                "unknown destructor remains rejected");
      } else if (scenario == 1) {
        f.guest().set_u32(t + 40, 0x400000);
        result = f.guest().call(0x40b170, {});
        require(!result && result.error.find("unowned work field") !=
                               std::string::npos,
                "non-heap pointer remains rejected");
      } else {
        std::string error;
        require(f.guest().add_override(
                    f.input + 0x8f20, [](X86Guest &) { return 0U; }, &error),
                error);
        result = f.guest().call(0x40b570, {t});
        require(!result && result.error.find("did not clear work") !=
                               std::string::npos,
                "destructor must clear mod resource");
      }
      ++negatives;
    }
    std::cout
        << "Input rumble task: real pinned DLL create, six original/native "
           "dispatches, destructor, CRT free, complete arena bytes, reused "
           "slot and repeated pool initialization match; "
        << negatives << " ownership negatives rejected\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
