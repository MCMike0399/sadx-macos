#include "platform/mac/metal_device_frame.hpp"
#import <Foundation/Foundation.h>
#include <array>
#include <iostream>
#include <stdexcept>
using namespace sadx;
using namespace sadx::graphics;
namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
Draw lines() {
    Draw draw;
    draw.space = PositionSpace::Screen;
    draw.viewport = {0, 0, 64, 64, 0, 1};
    draw.primitive_type = 2;
    draw.primitive_count = 2;
    draw.render_states = {{7, 0}, {137, 0}};
    draw.stage0_states = {{1, 3}, {3, 0}, {4, 3}, {6, 0}};
    for (const auto position : std::array<std::array<float, 4>, 4>{
             {{8, 16, 0.5F, 1}, {56, 16, 0.5F, 1},
              {8, 48, 0.5F, 1}, {56, 48, 0.5F, 1}}}) {
        Vertex vertex;
        vertex.position = position;
        vertex.diffuse = {1, 0, 0, 1};
        draw.vertices.push_back(vertex);
    }
    return draw;
}
}
int main() {
    @autoreleasepool {
        try {
            auto draw = lines();
            auto prepared = prepare_draw(draw);
            require(prepared.unsupported.empty(), "line list must be supported");
            require(prepared.indices == std::vector<std::uint32_t>{0, 1, 2, 3},
                    "two isolated line pairs");
            draw.indices = {3, 1, 2, 0};
            require(prepare_draw(draw).indices == draw.indices, "indexed endpoint order");
            draw.indices[3] = 9;
            require(prepare_draw(draw).unsupported == "index outside vertex range",
                    "invalid line endpoint reported");
            draw = lines();
            draw.primitive_count = 3;
            require(!prepare_draw(draw).unsupported.empty(), "short line stream reported");
            draw = lines();
            draw.has_normals = true;
            draw.render_states[137] = 1;
            draw.world[0] = 0;
            draw.space = PositionSpace::Object;
            require(prepare_draw(draw).unsupported == "singular visible normal transform",
                    "triangle zero-area exemption cannot discard visible lines");
            draw = lines();
            draw.primitive_type = 1;
            require(!prepare_draw(draw).unsupported.empty(), "points remain explicit unsupported");

            auto device = MTLCreateSystemDefaultDevice();
            require(device != nil, "Metal device");
            std::string error;
            auto renderer = MetalDeviceFrameRenderer::create(device, &error);
            require(bool(renderer), error.c_str());
            auto queue = [device newCommandQueue];
            auto descriptor = [MTLTextureDescriptor
                texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                             width:64 height:64 mipmapped:NO];
            descriptor.storageMode = MTLStorageModeShared;
            descriptor.usage = MTLTextureUsageRenderTarget;
            auto target = [device newTextureWithDescriptor:descriptor];
            auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
            pass.colorAttachments[0].texture = target;
            pass.colorAttachments[0].storeAction = MTLStoreActionStore;
            const auto pixel = [&](unsigned x, unsigned y) {
                std::array<std::uint8_t, 4> value{};
                [target getBytes:value.data() bytesPerRow:4
                     fromRegion:MTLRegionMake2D(x, y, 1, 1) mipmapLevel:0];
                return value;
            };
            const auto render = [&](const Draw &value) {
                auto command = [queue commandBuffer];
                auto stats = renderer->encode(Frame{{Clear{1, 0xff000000U, 0, 0}, value}},
                                              command, pass);
                [command commit];
                [command waitUntilCompleted];
                require(command.status == MTLCommandBufferStatusCompleted, "GPU completion");
                require(stats.draws == 1 && stats.unsupported.empty(), "line draw encoded");
            };
            draw = lines();
            render(draw);
            require(pixel(32, 16) == std::array<std::uint8_t, 4>{0, 0, 255, 255} &&
                        pixel(32, 48) == std::array<std::uint8_t, 4>{0, 0, 255, 255},
                    "both isolated lines rasterize");
            require(pixel(32, 32) == std::array<std::uint8_t, 4>{0, 0, 0, 255},
                    "line list does not join endpoint pairs or fill a triangle");
            draw.indices = {0, 1};
            draw.primitive_count = 1;
            render(draw);
            require(pixel(32, 16)[2] == 255 && pixel(32, 48)[2] == 0,
                    "indexed line count selects endpoints");
            draw.render_states[28] = 1;
            draw.render_states[35] = 3;
            draw.render_states[34] = 0xff0000ffU;
            draw.render_states[36] = 0;
            draw.render_states[37] = 0x3f800000U;
            render(draw);
            require(pixel(32, 16) == std::array<std::uint8_t, 4>{128, 0, 128, 255},
                    "line fragments share device-Z fog");
            draw = lines();
            draw.space = PositionSpace::Object;
            for (auto &vertex : draw.vertices) {
                vertex.position[0] = vertex.position[0] / 32 - 1;
                vertex.position[1] = 1 - vertex.position[1] / 32;
            }
            draw.world[12] = 0.25F;
            render(draw);
            require(pixel(12, 16)[2] == 0 && pixel(32, 16)[2] == 255,
                    "object line endpoints use world/view/projection transforms");
            std::cout << "Line lists: CPU endpoint validation and GPU isolated pairs, indices, fog passed.\n";
            return 0;
        } catch (const std::exception &error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
}
