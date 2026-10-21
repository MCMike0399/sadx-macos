#include "platform/mac/metal_device_frame.hpp"
#include "engine/native_message_texture.hpp"
#import <Foundation/Foundation.h>
#include <array>
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
            std::vector<texture_lifetime::Memory> pool(2);
            std::int32_t cached = -1;
            texture_lifetime::Memory *active = nullptr;
            const texture_lifetime::List *current = nullptr;
            std::array<std::uint32_t, 8> scratch{};
            texture_lifetime::State state{pool,cached,active,current,scratch};
            texture_palette::State palette;
            texture_lifetime::List list;
            std::uint16_t bitmap = 4660;
            require(message_image::load(list,7,&bitmap,1,1,state,palette)==1,"MSG load");
            auto first = triangle();
            first.texture = list.entries[0]->surface->texture;
            first.texture_coordinates = 1;
            first.stage0_states = {{1,2},{2,2},{4,2},{5,2},{16,1},{17,1}};
            first.render_states[27] = 0;
            first.render_states[15] = 0;
            const Clear black{3,4278190080U,1065353216U,0};
            auto stats = render(Frame{{black,first}});
            require(stats.textured==1 && stats.unsupported.empty(),"MSG encoded");
            require(pixel(16,40)==std::array<std::uint8_t,4>{68,51,34,17},"4444 GPU first readback");
            bitmap = 43981;
            require(message_image::reload(7,&bitmap,state,palette)==1,"MSG reload");
            auto second = first;
            second.texture = list.entries[0]->surface->texture;
            texture_lifetime::Services services{[] {}};
            texture_lifetime::release_list(&list,state,services);
            require(!pool[0].surface,"manager release");
            stats=render(Frame{{black,second}});
            require(pixel(16,40)==std::array<std::uint8_t,4>{221,204,187,170},"4444 GPU reload readback after release");
            stats=render(Frame{{black,first}});
            require(pixel(16,40)==std::array<std::uint8_t,4>{68,51,34,17},"immutable old snapshot after reload/release");
            std::cout << "three MSG Metal4444 readbacks passed, original/reloaded/released snapshots exact\n";
        } catch (const std::exception &error) {
            std::cerr << error.what() << "\n";
            return 1;
        }
    }
}
