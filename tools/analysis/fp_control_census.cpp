// Development reference only: observe entry registers without replacing game behavior.
#include "guest/fpu_shims.hpp"
#include "hle/hle_device_bridge.hpp"
#include "hle/hle_process.hpp"
#include <unicorn/unicorn.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>

namespace {
using namespace sadx;
void require(bool value, const std::string& message) {
    if (!value) throw std::runtime_error(message);
}
struct Site {
    std::string name;
    bool bound = false;
    std::map<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t, bool>, std::uint64_t> samples;
    std::uint64_t missing_return = 0;
    void observe(X86Guest& guest, bool device_created = false) {
        const auto caller = guest.u32(guest.esp());
        if (!caller) ++missing_return;
        ++samples[{guest.reg(UC_X86_REG_FPCW), guest.mxcsr(), caller.value_or(0), device_created}];
    }
};
struct Hash {
    std::uint64_t value = 1469598103934665603ULL;
    void bytes(const void* data, std::size_t size) {
        const auto* p = static_cast<const std::uint8_t*>(data);
        for (std::size_t i = 0; i < size; ++i) {
            value ^= p[i];
            value *= 1099511628211ULL;
        }
    }
    template<class T> void add(const T& item) { bytes(&item, sizeof item); }
};
void self_test() {
    std::string error;
    auto guest = X86Guest::create(&error);
    require(bool(guest), error);
    require(guest->map(0x100000, 0x2000, true, &error), error);
    // FLDCW [101000]; LDMXCSR [101004]; MOV EAX,12345678; RET.
    const std::uint8_t code[] = {0xd9, 0x2d, 0, 0x10, 0x10, 0,
        0x0f, 0xae, 0x15, 4, 0x10, 0x10, 0, 0xb8, 0x78, 0x56, 0x34, 0x12, 0xc3};
    require(guest->write(0x100000, code, sizeof code), "test code write");
    require(guest->set_u32(0x101000, 0x7f) && guest->set_u32(0x101004, 0x5f80), "test controls");
    const auto plain = guest->call(0x100000, {});
    require(bool(plain) && plain.eax == 0x12345678, "untraced control fixture");
    Site site{"fixture", false, {}, 0};
    require(guest->add_instruction_hook(0x10000d, [&site](X86Guest& g, std::uint32_t) {
        site.observe(g);
        return false;
    }, &error), error);
    const auto traced = guest->call(0x100000, {});
    require(bool(traced) && traced.eax == plain.eax, "trace changes source execution");
    require(guest->set_u32(0x101000, 0x37f) && guest->set_u32(0x101004, 0x3f80), "second controls");
    require(bool(guest->call(0x100000, {})), "second sample");
    require(site.samples.size() == 2 && site.missing_return == 0, "sample separation");
    require(site.samples.at({0x7f, 0x5f80, guest->stop_address(), false}) == 1 &&
            site.samples.at({0x37f, 0x3f80, guest->stop_address(), false}) == 1, "sample values");
    std::cout << "{\"passed\":true,\"samples\":2,\"trace_is_read_only\":true}\n";
}
void run(int argc, char** argv) {
    require(argc == 8, "usage: fp-census GAME SYSTEM TARGETS PROFILE VARIANT FRAMES OVERLAY");
    const std::string dir = argv[1], system = argv[2], profile = argv[4], variant = argv[5];
    require(profile == "intro" || profile == "controller" || profile == "stage", "unknown profile");
    require(variant == "plain" || variant == "original" || variant == "native", "unknown variant");
    const auto frames = std::stoul(argv[6]);
    std::ifstream input(dir + "/sonic.exe", std::ios::binary);
    require(bool(input), "missing sonic.exe");
    std::string error;
    const auto image = ExecutableImage::parse({std::istreambuf_iterator<char>(input), {}}, &error);
    require(bool(image), error);
    // Each invocation owns a fresh overlay; callers provide a unique build path.
    require(!std::filesystem::exists(argv[7]), "overlay must be fresh");
    auto process = HleProcess::create(dir, argv[7], &error);
    require(bool(process), error);
    if (profile == "controller" || profile == "stage") process->host_xinput(HleXInputGamepad{});
    process->use_real_cpp_runtime(system);
    process->hle().set_logging(false);
    require(process->map_main(*image, &error), error);
    FpuShimStats shims;
    require(install_fpu_shims(process->guest(), load_fpu_sites("reference/inventory/fpu-sites.json"),
                              shims, &error), error);
    std::map<std::uint32_t, Site> sites;
    std::ifstream targets(argv[3]);
    require(bool(targets), "missing targets");
    std::string at, name;
    while (targets >> at >> name) sites.emplace(std::stoul(at, nullptr, 16), Site{name, false, {}, 0});
    const auto entries = process_overrides(*process, *image);
    for (const auto& entry : entries) {
        auto& site = sites[entry.address];
        site.bound = true;
        if (site.name.empty()) site.name = entry.name;
        if (variant == "native") {
            const auto bridge = entry.bridge;
            require(process->guest().add_override(entry.address, [&site, bridge, &process](X86Guest& g) {
                site.observe(g, !process->d3d8_presentation.empty());
                return bridge(g);
            }, &error), error);
        }
    }
    if (variant != "plain") {
        for (auto& [entry, site] : sites) {
            if (variant == "native" && site.bound) continue;
            auto* const observed = &site;
            require(process->guest().add_instruction_hook(entry, [observed, &process](X86Guest& g, std::uint32_t) {
                observed->observe(g, !process->d3d8_presentation.empty());
                return false;  // Execute the original instruction unchanged.
            }, &error), error);
        }
    }
    std::size_t polygons_seen = 0;
    std::vector<std::uint64_t> digests, clocks;
    std::vector<std::pair<std::size_t, std::uint32_t>> modes, stages;
    process->on_present = [&](X86Guest& guest) {
        const auto& device = process->d3d8;
        Hash h;
        for (const auto& [key, value] : device.render_states) { h.add(key); h.add(value); }
        for (const auto& [key, value] : device.stage0_states) { h.add(key); h.add(value); }
        for (const auto& [key, matrix] : device.transforms) { h.add(key); h.bytes(matrix.data(), 64); }
        for (; polygons_seen < device.polygons.size(); ++polygons_seen) {
            const auto& polygon = device.polygons[polygons_seen];
            h.add(polygon.primitive_type);
            h.add(polygon.primitive_count);
            h.bytes(polygon.vertex_bytes.data(), polygon.vertex_bytes.size());
        }
        h.add(device.begin_scenes); h.add(device.end_scenes); h.add(device.clears);
        h.add(device.render_calls); h.add(device.texture_binds);
        h.add(process->hle().calls()); h.add(process->now_100ns);
        digests.push_back(h.value);
        clocks.push_back(process->now_100ns);
        const auto mode = guest.u32(0x3abdc7c);
        require(mode.has_value(), "missing mode");
        if (modes.empty() || modes.back().second != *mode) modes.push_back({digests.size(), *mode});
        const auto frame = digests.size();
        std::uint16_t stage = 0;
        require(guest.read(0x3b22dcc, &stage, sizeof stage), "missing stage");
        if (stages.empty() || stages.back().second != stage) stages.push_back({frame, stage});
        if (profile == "stage") {
            constexpr std::size_t presses[] = {450, 800, 950, 1100, 1250, 1400, 1550, 1700, 1850};
            for (const auto press : presses) {
                if (frame == press) process->host_key(13, true);
                if (frame == press + 5) process->host_key(13, false);
            }
        }
        if (profile == "controller") {
            if (frame == 450 || frame == 800)
                process->host_xinput(HleXInputGamepad{0x10, 0, 0, 0, 0, 0, 0});
            if (frame == 455 || frame == 805) process->host_xinput(HleXInputGamepad{});
        }
        if (frame % 500 == 0) std::cerr << profile << '/' << variant << " frame " << frame << '\n';
        if (frame >= frames) guest.stop("census frames done");
    };
    const auto result = process->guest().call(0x643f96, {}, std::nullopt, std::nullopt, 0x27f, 0);
    std::cout << "{\"profile\":" << std::quoted(profile) << ",\"variant\":" << std::quoted(variant)
              << ",\"stop\":" << std::quoted(result ? "returned" : result.error)
              << ",\"frames\":" << digests.size() << ",\"digests\":[";
    for (std::size_t i = 0; i < digests.size(); ++i) std::cout << (i ? "," : "") << digests[i];
    std::cout << "],\"clocks\":[";
    for (std::size_t i = 0; i < clocks.size(); ++i) std::cout << (i ? "," : "") << clocks[i];
    std::cout << "],\"modes\":[";
    for (std::size_t i = 0; i < modes.size(); ++i)
        std::cout << (i ? "," : "") << '[' << modes[i].first << ',' << modes[i].second << ']';
    std::cout << "],\"stages\":[";
    for (std::size_t i = 0; i < stages.size(); ++i)
        std::cout << (i ? "," : "") << '[' << stages[i].first << ',' << stages[i].second << ']';
    std::cout << "],\"sites\":[";
    bool comma = false;
    for (const auto& [entry, site] : sites) {
        std::cout << (comma ? "," : "") << "{\"address\":" << entry << ",\"name\":"
                  << std::quoted(site.name) << ",\"bound\":" << (site.bound ? "true" : "false")
                  << ",\"missing_return\":" << site.missing_return << ",\"samples\":[";
        bool sample_comma = false;
        for (const auto& [key, count] : site.samples) {
            const auto& [cw, mxcsr, caller, device_created] = key;
            std::cout << (sample_comma ? "," : "") << '[' << cw << ',' << mxcsr << ',' << caller << ',' << count << ',' << (device_created ? 1 : 0) << ']';
            sample_comma = true;
        }
        std::cout << "]}";
        comma = true;
    }
    std::cout << "]}\n";
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--self-test") self_test();
        else run(argc, argv);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
