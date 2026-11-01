// Exercise the actual HleDshow consumer, not a model of its duration formula.
#include "hle/hle_process.hpp"
#include "mpeg_timing_stream.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool ok, const std::string &why) {
    if (!ok)
        throw std::runtime_error(why);
}
constexpr std::uint32_t kScratch = 0x10000000;

void run(unsigned rate) {
    const auto directory = std::filesystem::absolute("build/mpeg-timing-hle-fixtures");
    std::filesystem::create_directories(directory);
    auto bytes = mpeg_fixture::stream(rate, 37245, 937245);
    if (rate == 0)
        bytes = {0, 0, 1, 0xe0, 0, 5, 0x21, 0, 1, 0, 1};
    {
        std::ofstream file(directory / "synthetic.mpg", std::ios::binary);
        file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
        require(bool(file), "write synthetic stream");
    }
    std::string error;
    auto process = sadx::HleProcess::create(directory, directory / "overlay", &error);
    require(bool(process), error);
    auto &guest = process->guest();
    require(guest.map(kScratch, 4096, false, &error), error);
    const auto put_guid = [&](std::uint32_t at, std::uint32_t first, bool clsid = false) {
        const std::uint8_t tail_interface[] = {0xd4, 0x0a, 0xce, 0x11, 0xb0, 0x3a,
                                               0,    0x20, 0xaf, 0x0b, 0xa7, 0x70};
        const std::uint8_t tail_class[] = {0x4f, 0x52, 0xce, 0x11, 0x9f, 0x53,
                                           0,    0x20, 0xaf, 0x0b, 0xa7, 0x70};
        require(guest.set_u32(at, first), "GUID first word");
        require(guest.write(at + 4, clsid ? tail_class : tail_interface, 12), "GUID tail");
    };
    put_guid(kScratch, 0xe436ebb3, true);
    put_guid(kScratch + 16, 0x56a868a9);
    const auto create = process->hle().trap(guest, "ole32.dll", "CoCreateInstance", &error);
    require(bool(create), error);
    const auto result = guest.call(*create, {kScratch, 0, 1, kScratch + 16, kScratch + 64});
    require(bool(result) && result.eax == 0, "create graph: " + result.error);
    const auto graph = guest.u32(kScratch + 64).value();
    const auto call = [&](const std::string &interface, const std::string &method,
                          std::vector<std::uint32_t> arguments) {
        const auto trap = process->hle().trap(guest, "com:" + interface, method, &error);
        require(bool(trap), error);
        const auto called = guest.call(*trap, arguments);
        std::cout << rate << ' ' << interface << "::" << method << ' ' << bool(called) << ' '
                  << called.eax << ' ' << called.error << '\n';
        return called;
    };
    const auto query = [&](std::uint32_t iid) {
        put_guid(kScratch + 16, iid);
        const auto called =
            call("IGraphBuilder", "QueryInterface", {graph, kScratch + 16, kScratch + 64});
        require(bool(called) && called.eax == 0, "query graph interface");
        return guest.u32(kScratch + 64).value();
    };
    const auto position = query(0x56a868b2);
    const auto control = query(0x56a868b1);
    const auto event = query(0x56a868c0);
    require(call("IMediaPosition", "get_Duration", {position, kScratch + 128}).eax == 0x80004005U,
            "duration absent before source");
    require(process->put_wide(kScratch + 256, "synthetic.mpg"), "movie path");
    const auto added =
        call("IGraphBuilder", "AddSourceFilter", {graph, kScratch + 256, 0, kScratch + 64});
    if (rate == 0) {
        require(!added && added.error.find("IGraphBuilder::AddSourceFilter(synthetic.mpg): "
                                           "no video sequence header in the packet payloads") !=
                              std::string::npos,
                "parser failure retains consumer prefix");
        return;
    }
    require(bool(added) && added.eax == 0, "add synthetic source");
    const std::uint32_t numerator[] = {0, 24000, 24, 25, 30000, 30, 50, 60000, 60};
    const std::uint32_t denominator[] = {1, 1001, 1, 1, 1001, 1, 1, 1001, 1};
    const auto duration = 100000000ULL + 10000000ULL * denominator[rate] / numerator[rate];
    const auto seconds = [&](const std::string &method, double expected) {
        const auto called = call("IMediaPosition", method, {position, kScratch + 128});
        require(bool(called) && called.eax == 0, method);
        double value;
        require(guest.read(kScratch + 128, &value, sizeof(value)), "read double");
        std::uint64_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        std::cout << method << " bits " << bits << '\n';
        require(value == expected, "exact " + method);
    };
    seconds("get_Duration", static_cast<double>(duration) / 1e7);
    seconds("get_CurrentPosition", 0);
    require(call("IMediaControl", "Run", {control}).eax == 0, "run");
    const auto get_event = [&] {
        return call("IMediaEventEx", "GetEvent",
                    {event, kScratch + 160, kScratch + 164, kScratch + 168, 0});
    };
    process->now_100ns += duration - 1;
    seconds("get_CurrentPosition", static_cast<double>(duration - 1) / 1e7);
    require(get_event().eax == 0x80004004U, "not complete one tick before boundary");
    ++process->now_100ns;
    seconds("get_CurrentPosition", static_cast<double>(duration) / 1e7);
    require(get_event().eax == 0, "complete at exact boundary");
    require(guest.u32(kScratch + 160) == 1 && guest.u32(kScratch + 164) == 0 &&
                guest.u32(kScratch + 168) == 0,
            "EC_COMPLETE and parameters");
    require(get_event().eax == 0x80004004U, "completion delivered once");
    process->now_100ns += duration;
    seconds("get_CurrentPosition", static_cast<double>(duration) / 1e7);
    require(call("IMediaControl", "Pause", {control}).eax == 0, "pause");
    require(call("IMediaControl", "Stop", {control}).eax == 0, "stop");
    // Replacing the source resets position and allows completion to be queued again.
    require(
        call("IGraphBuilder", "AddSourceFilter", {graph, kScratch + 256, 0, kScratch + 64}).eax ==
            0,
        "reload");
    seconds("get_CurrentPosition", 0);
    for (const auto &entry : process->file_trace)
        std::cout << "file " << entry << '\n';
    bool assumption_found = false;
    for (const auto &assumption : process->assumptions) {
        if (assumption.find("DirectShow") != std::string::npos)
            std::cout << "assumption " << assumption << '\n';
        if (assumption.find("splitter's own rule was not captured") != std::string::npos)
            assumption_found = true;
    }
    require(assumption_found, "uncaptured splitter assumption remains explicit");
}
} // namespace

int main() {
    try {
        for (unsigned rate = 1; rate <= 8; ++rate)
            run(rate);
        run(0);
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
