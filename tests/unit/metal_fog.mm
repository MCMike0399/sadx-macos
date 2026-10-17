#include "platform/mac/metal_device_frame.hpp"
#import <Foundation/Foundation.h>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace sadx;
using namespace sadx::graphics;
namespace {
void require(bool value, const char *why) {
    if (!value)
        throw std::runtime_error(why);
}
Draw triangle(float z = 0.5F, std::array<float, 4> color = {1, 0, 0, 1}) {
    Draw draw;
    draw.viewport = {0, 0, 64, 64, 0, 1};
    draw.primitive_count = 1;
    draw.render_states = {{7, 0}, {22, 1}, {137, 0}};
    draw.stage0_states = {{1, 3}, {3, 0}, {4, 3}, {6, 0}};
    for (auto position : std::array<std::array<float, 4>, 3>{
             {{-0.75F, -0.75F, z, 1}, {0.75F, -0.75F, z, 1}, {-0.75F, 0.75F, z, 1}}}) {
        Vertex vertex;
        vertex.position = position;
        vertex.diffuse = color;
        draw.vertices.push_back(vertex);
    }
    return draw;
}
} // namespace
int main() {
    @autoreleasepool {
        try {
            auto device = MTLCreateSystemDefaultDevice();
            require(device != nil, "Metal device");
            std::string error;
            auto renderer = MetalDeviceFrameRenderer::create(device, &error);
            require(bool(renderer), error.c_str());
            auto queue = [device newCommandQueue];
            auto color_descriptor =
                [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                   width:64
                                                                  height:64
                                                               mipmapped:NO];
            color_descriptor.storageMode = MTLStorageModeShared;
            color_descriptor.usage = MTLTextureUsageRenderTarget;
            auto target = [device newTextureWithDescriptor:color_descriptor];
            auto depth_descriptor =
                [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                   width:64
                                                                  height:64
                                                               mipmapped:NO];
            depth_descriptor.storageMode = MTLStorageModePrivate;
            depth_descriptor.usage = MTLTextureUsageRenderTarget;
            auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
            pass.colorAttachments[0].texture = target;
            pass.colorAttachments[0].storeAction = MTLStoreActionStore;
            pass.depthAttachment.texture = [device newTextureWithDescriptor:depth_descriptor];
            pass.depthAttachment.storeAction = MTLStoreActionStore;
            const auto render = [&](Frame frame) {
                auto command = [queue commandBuffer];
                auto stats = renderer->encode(frame, command, pass);
                [command commit];
                [command waitUntilCompleted];
                require(command.status == MTLCommandBufferStatusCompleted, "GPU completion");
                return stats;
            };
            const auto pixel = [&](unsigned x, unsigned y) {
                std::array<std::uint8_t, 4> result{};
                [target getBytes:result.data()
                     bytesPerRow:4
                      fromRegion:MTLRegionMake2D(x, y, 1, 1)
                     mipmapLevel:0];
                return result;
            };
            const Clear black{3, 0xff000000U, 0x3f800000U, 0};
            auto draw = triangle();
            draw.space = PositionSpace::Screen;
            draw.render_states[28] = 1;
            draw.render_states[34] = 0xff0000ffU;
            draw.render_states[140] = 3;
            // Equal W: screen barycentric weights at (24,24) are (1/3,1/3,1/3).
            draw.vertices[0].position = {8, 8, 0.5F, 1};
            draw.vertices[1].position = {56, 8, 0.5F, 1};
            draw.vertices[2].position = {8, 56, 0.5F, 1};
            for (auto &v : draw.vertices) {
                v.diffuse[3] = 128.0F / 255;
                v.specular[3] = 0;
            }
            draw.vertices[1].specular[3] = 1;
            auto stats = render(Frame{{black, draw}});
            require(stats.draws == 1 && stats.unsupported.empty(), "supplied vertex fog encoded");
            const auto close = [&](std::array<unsigned, 4> expected, const char *why) {
                const auto actual = pixel(24, 24);
                for (unsigned i = 0; i < 4; ++i)
                    require(std::abs(int(actual[i]) - int(expected[i])) <= 1, why);
            };
            close({170, 0, 85, 128}, "interpolated fog factor preserves diffuse alpha");
            draw.has_specular = false;
            render(Frame{{black, draw}});
            close({0, 0, 255, 128}, "missing supplied factor leaves color unfogged");
            // Generated exponential vertex fog: depths 0, 2, 4; average of exponentials.
            // The projection puts each vertex at the same screen positions as above.
            draw = triangle();
            draw.render_states[28] = 1;
            draw.render_states[34] = 0xff0000ffU;
            draw.render_states[140] = 1;
            draw.projection[10] = 0;
            draw.projection[14] = 0.5F;
            draw.vertices[0].position = {-0.75F, 0.75F, 0, 1};
            draw.vertices[1].position = {0.75F, 0.75F, 2, 1};
            draw.vertices[2].position = {-0.75F, -0.75F, 4, 1};
            stats = render(Frame{{black, draw}});
            require(stats.draws == 1 && stats.unsupported.empty(), "generated vertex fog encoded");
            // (1 + exp(-2) + exp(-4))/3 gives red 98; per-pixel exp(-2) gives 35.
            close({157, 0, 98, 255}, "formula evaluated per vertex before interpolation");
            draw.render_states[140] = 2;
            render(Frame{{black, draw}});
            close({168, 0, 87, 255}, "exponential squared reaches GPU");
            draw.render_states[140] = 3;
            float end = 4;
            std::uint32_t end_bits;
            std::memcpy(&end_bits, &end, 4);
            draw.render_states[37] = end_bits;
            render(Frame{{black, draw}});
            close({128, 0, 128, 255}, "linear vertex fog reaches GPU");
            draw.render_states[48] = 1;
            stats = render(Frame{{black, draw}});
            require(stats.draws == 1 && stats.unsupported.empty(), "range fog accepted");
            require(pixel(24, 24)[2] < 128, "range fog increases distance off camera axis");
            // Table fog must win over both the supplied alpha and vertex formula.
            draw.space = PositionSpace::Screen;
            draw.vertices[0].position = {8, 8, 0.5F, 1};
            draw.vertices[1].position = {56, 8, 0.5F, 1};
            draw.vertices[2].position = {8, 56, 0.5F, 1};
            draw.projection[11] = 1;
            draw.projection[15] = 0;
            draw.render_states[35] = 3;
            end = 2;
            std::memcpy(&end_bits, &end, 4);
            draw.render_states[37] = end_bits;
            stats = render(Frame{{black, draw}});
            require(stats.draws == 1 && stats.unsupported.empty(), "table precedence encoded");
            close({128, 0, 128, 255}, "table fog overrides vertex and range fog");
            std::cout << "Metal fog: supplied, generated, range, alpha and precedence pass\n";
            return 0;
        } catch (const std::exception &error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
}
