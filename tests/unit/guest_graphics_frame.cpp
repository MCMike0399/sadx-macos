#include "engine/graphics_frame.hpp"
#include "hle/hle_process.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace {
void require(bool value, const std::string &why) {
    if (!value)
        throw std::runtime_error(why);
}
} // namespace
int main() {
    try {
        std::string error;
        auto process = sadx::HleProcess::create(".", "build/guest-graphics-overlay", &error);
        require(bool(process), error);
        auto &guest = process->guest();
        const auto factory = process->hle().trap(guest, "d3d8.dll", "Direct3DCreate8", &error);
        require(factory.has_value(), error);
        auto created = guest.call(*factory, {0xdc});
        require(bool(created), created.error);
        const auto parameters = process->memory().allocate(52),
                   output = process->memory().allocate(16);
        require(parameters && output, "device memory");
        const std::uint32_t pp[13] = {64, 64, 22, 1, 0, 1, 0, 1, 1, 75, 0, 0, 0};
        guest.write(*parameters, pp, sizeof pp);
        const auto make = process->hle().trap(guest, "com:IDirect3D8", "CreateDevice", &error);
        require(make.has_value(), error);
        created = guest.call(*make, {created.eax, 0, 1, 0, 0x20, *parameters, *output});
        require(bool(created) && created.eax == 0, created.error);
        const auto device = guest.u32(*output).value_or(0);
        require(device != 0, "device identity");
        const auto call = [&](const char *name, std::initializer_list<std::uint32_t> args) {
            const auto method = process->hle().trap(guest, "com:IDirect3DDevice8", name, &error);
            require(method.has_value(), error);
            const auto result = guest.call(*method, args);
            require(bool(result) && result.eax == 0, result.error + name);
        };
        struct Vertex {
            float x, y, z;
            std::uint32_t color;
            float u, v;
        };
        const Vertex vertices[] = {
            {0, 0, 0.5F, 0xffffffffU, 0, 0}, {1, 0, 0.5F, 0xffffffffU, 0, 0},
            {2, 0, 0.5F, 0xffffffffU, 0, 0}, {3, 0, 0.5F, 0xffffffffU, 0, 0},
            {4, 0, 0.5F, 0xffffffffU, 0, 0}, {5, 0, 0.5F, 0xffffffffU, 0, 0}};
        call("CreateVertexBuffer", {device, sizeof vertices, 0, 322, 0, *output});
        const auto vb = guest.u32(*output).value_or(0);
        const auto vertex_memory = guest.u32(vb + 24).value_or(0);
        require(guest.write(vertex_memory, vertices, sizeof vertices), "vertex payload");
        const std::uint16_t indices[] = {99, 1, 2, 3, 99};
        call("CreateIndexBuffer", {device, sizeof indices, 0, 101, 0, *output});
        const auto ib = guest.u32(*output).value_or(0);
        const auto index_memory = guest.u32(ib + 24).value_or(0);
        require(guest.write(index_memory, indices, sizeof indices), "index payload");
        const auto matrix_memory = process->memory().allocate(64),
                   viewport_memory = process->memory().allocate(24);
        require(matrix_memory && viewport_memory, "state memory");
        auto matrix = sadx::graphics::kIdentity;
        matrix[12] = 3;
        guest.write(*matrix_memory, matrix.data(), 64);
        call("SetTransform", {device, 256, *matrix_memory});
        const std::uint32_t viewport[6] = {0, 0, 64, 64, 0, 0x3f800000U};
        guest.write(*viewport_memory, viewport, sizeof viewport);
        call("SetViewport", {device, *viewport_memory});
        call("LightEnable", {device, 37, 1});
        const auto light_memory = process->memory().allocate(104);
        require(light_memory.has_value(), "light memory");
        call("GetLight", {device, 37, *light_memory});
        require(guest.u32(*light_memory) == 3 && guest.u32(*light_memory + 4) == 0x3f800000U &&
                    guest.u32(*light_memory + 72) == 0x3f800000U,
                "implicit directional light contract");
        call("SetVertexShader", {device, 322});
        call("SetStreamSource", {device, 0, vb, sizeof(Vertex)});
        call("SetIndices", {device, ib, 1});
        call("DrawIndexedPrimitive", {device, 4, 1, 3, 1, 1});
        require(process->frame.draws.size() == 1, "buffer draw recorded");
        const auto &indexed = process->frame.draws[0];
        require(indexed.unsupported.empty() &&
                    indexed.indices == std::vector<std::uint32_t>{0, 1, 2},
                "16-bit indices rebased from declared minimum");
        float first = 0;
        std::memcpy(&first, indexed.vertices.data(), 4);
        require(first == 2, "base vertex plus minimum index");
        require(indexed.transforms.at(256)[12] == 3 && indexed.viewport &&
                    (*indexed.viewport)[2] == 64,
                "per-draw state");
        matrix[12] = 5;
        guest.write(*matrix_memory, matrix.data(), 64);
        call("SetTransform", {device, 256, *matrix_memory});
        call("Clear", {device, 0, 0, 3, 0xff000000U, 0x3f800000U, 0});
        call("DrawPrimitive", {device, 4, 1, 1});
        require(process->frame.draws[0].transforms.at(256)[12] == 3 &&
                    process->frame.draws[1].transforms.at(256)[12] == 5,
                "later matrix updates do not rewrite old draws");
        require(process->frame.events.size() == 3 && process->frame.events[0].draw &&
                    !process->frame.events[1].draw && process->frame.events[2].draw,
                "clear/draw event order");
        call("DrawPrimitiveUP", {device, 4, 1, vertex_memory, sizeof(Vertex)});
        call("GetStreamSource", {device, 0, *output, *output + 4});
        require(guest.u32(*output) == 0, "UP clears bound stream zero");
        const std::uint32_t user_indices[] = {1, 3, 2};
        const auto user_memory = process->memory().allocate(sizeof user_indices);
        require(user_memory.has_value(), "index memory");
        guest.write(*user_memory, user_indices, sizeof user_indices);
        call("DrawIndexedPrimitiveUP",
             {device, 4, 1, 3, 1, *user_memory, 102, vertex_memory, sizeof(Vertex)});
        require(process->frame.draws.back().indices == std::vector<std::uint32_t>{0, 2, 1},
                "32-bit user indices with minimum vertex");
        call("GetIndices", {device, *output, *output + 4});
        require(guest.u32(*output) == 0, "indexed UP clears index binding");
        require(guest.set_u32(vertex_memory, 0x7fc00000U), "overwrite source after draw");
        float copied = 1;
        std::memcpy(&copied, process->frame.draws[2].vertices.data(), 4);
        require(copied == 0, "owned vertex bytes survive later guest mutation");
        call("SetStreamSource", {device, 0, vb, sizeof(Vertex)});
        call("SetIndices", {device, ib, 0});
        call("DrawIndexedPrimitive", {device, 4, 1, 3, 0, 1});
        require(!process->frame.draws.back().unsupported.empty(),
                "invalid index range is explicit");
        std::cout
            << "Guest graphics: all four draw entry points, buffers, index widths/base/minimum, "
               "state snapshots, ordered clears and UP binding resets passed.\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
