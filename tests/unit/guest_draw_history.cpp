#include "hle/hle_process.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
}
int main() {
    std::string error;
    auto process = sadx::HleProcess::create(".", "build/guest-draw-history-overlay", &error);
    require(bool(process), "create process");
    process->hle().set_logging(false);
    auto &guest = process->guest();
    const auto factory = process->hle().trap(guest, "d3d8.dll", "Direct3DCreate8", &error);
    require(factory.has_value(), "D3D factory");
    const auto d3d = guest.call(*factory, {220});
    require(bool(d3d) && d3d.eax, "create D3D owner");
    const auto parameters = process->memory().allocate(52);
    const auto output = process->memory().allocate(4);
    require(parameters && output, "device parameters");
    const std::uint32_t presentation[13] = {640, 480, 22, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0};
    require(guest.write(*parameters, presentation, sizeof presentation), "write device parameters");
    const auto vtable = guest.u32(d3d.eax);
    const auto create = vtable ? guest.u32(*vtable + 15 * 4) : std::nullopt;
    require(create.has_value(), "device factory");
    const auto device = guest.call(*create, {d3d.eax, 0, 1, 0, 32, *parameters, *output});
    require(bool(device) && device.eax == 0, "create device");
    const auto vertices = process->memory().allocate(3456);
    const auto stack = process->memory().allocate(24);
    require(vertices && stack, "allocate draw fixture");
    std::vector<std::uint8_t> bytes(3456, 37);
    require(process->guest().write(*vertices, bytes.data(), bytes.size()), "write vertices");
    const auto invoke = [&](const char *name, const std::vector<std::uint32_t> &arguments) {
        std::vector<std::uint32_t> words{0, 0};
        words.insert(words.end(), arguments.begin(), arguments.end());
        require(words.size() <= 6, "fixture stack capacity");
        require(process->guest().write(*stack, words.data(), words.size() * 4), "write call");
        const auto *method = process->hle().find("com:IDirect3DDevice8", name);
        require(method != nullptr, "registered device method");
        sadx::HleCall call(process->guest(), *stack);
        require(method->body(call) == 0, "device method succeeds");
    };
    process->vertex_shader = 0x142;
    invoke("DrawPrimitiveUP", {4, 48, *vertices, 24});
    invoke("Present", {0, 0, 0, 0});
    require(process->d3d8.polygons.size() == 1, "default preserves oracle draw history");
    process->capture_legacy_draw_history = false;
    for (unsigned frame = 0; frame < 2000; ++frame) {
        invoke("DrawPrimitiveUP", {4, 48, *vertices, 24});
        invoke("Present", {0, 0, 0, 0});
        require(process->last_frame.draws.size() == 1, "last frame remains complete");
        require(process->last_frame.draws[0].vertices == bytes, "rendered vertices unchanged");
    }
    std::cout << "legacy_draws=" << process->d3d8.polygons.size()
              << " expected=1, presented_frames=" << process->d3d8.presents << '\n';
    require(process->d3d8.polygons.size() == 1, "interactive observer retains historical vertex buffers");
}
