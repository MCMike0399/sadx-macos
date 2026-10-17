#include "platform/mac/metal_device_frame.hpp"
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
            const Clear black{3, 0xff000000U, 0x3f800000U, 0}, blue{3, 0xff0000ffU, 0x3f800000U, 0};
            auto red = triangle();
            auto stats = render(Frame{{black, red}});
            require(stats.draws == 1 && stats.unsupported.empty(), "native object draw encoded");
            require(pixel(16, 40) == std::array<std::uint8_t, 4>{0, 0, 255, 255},
                    "native 3D triangle readback");
            red.world[12] = 0.5F;
            render(Frame{{black, red}});
            require(pixel(16, 40) == std::array<std::uint8_t, 4>{0, 0, 0, 255},
                    "world translation applied");
            red = triangle(0.25F);
            auto green = triangle(0.75F, {0, 1, 0, 1});
            for (auto *draw : {&red, &green}) {
                draw->render_states[7] = 1;
                draw->render_states[14] = 1;
                draw->render_states[23] = 2;
            }
            render(Frame{{black, red, green}});
            require(pixel(16, 40) == std::array<std::uint8_t, 4>{0, 0, 255, 255},
                    "less depth rejects farther draw");
            green.world[12] = 0.5F;
            render(Frame{{black, red, blue, green}});
            require(pixel(16, 40) == std::array<std::uint8_t, 4>{255, 0, 0, 255},
                    "interleaved clear preserves command order");
            red = triangle(0.5F, {1, 0, 0, 64.0F / 255});
            red.render_states[15] = 1;
            red.render_states[24] = 128;
            red.render_states[25] = 7;
            render(Frame{{blue, red}});
            require(pixel(16, 40) == std::array<std::uint8_t, 4>{255, 0, 0, 255},
                    "alpha comparison discards fragment");
            red = triangle(1.5F);
            render(Frame{{blue, red}});
            require(pixel(16, 40) == std::array<std::uint8_t, 4>{255, 0, 0, 255},
                    "homogeneous far clipping");
            red = triangle();
            red.render_states[22] = 2;
            render(Frame{{black, red}});
            const auto clockwise = pixel(16, 40);
            red.render_states[22] = 3;
            render(Frame{{black, red}});
            const auto counter = pixel(16, 40);
            require(clockwise != counter, "opposite culling modes select opposite windings");
            red = triangle();
            const auto original = red.vertices;
            for (unsigned index = 0; index < 200; ++index)
                red.vertices.insert(red.vertices.end(), original.begin(), original.end());
            red.primitive_count = 201;
            stats = render(Frame{{black, red}});
            require(stats.draws == 1 && stats.unsupported.empty() && pixel(16, 40)[2] == 255,
                    "vertex buffers larger than setVertexBytes limit");
            // Analytic perspective fixture: at (24,24), screen barycentrics are 1/3 each.
            // u = (.25/3)/(1/3+.25/3+1/3) = 1/9, selecting red texel 0;
            // affine interpolation selects blue texel 2 instead.
            Draw perspective = triangle();
            perspective.space = PositionSpace::Screen;
            perspective.texture_coordinates = 1;
            perspective.vertices[0].position = {8, 8, 0.5F, 1};
            perspective.vertices[0].uv[0] = {0, 0};
            perspective.vertices[1].position = {56, 8, 0.5F, 0.25F};
            perspective.vertices[1].uv[0] = {1, 0};
            perspective.vertices[2].position = {8, 56, 0.5F, 1};
            perspective.vertices[2].uv[0] = {0, 0};
            perspective.stage0_states = {{1, 2},  {2, 2},  {4, 2},  {5, 2},
                                         {13, 3}, {14, 3}, {16, 1}, {17, 1}};
            auto texture = std::make_shared<Texture>();
            texture->width = 8;
            texture->height = 1;
            texture->bgra = {0, 0, 255, 255, 0, 255, 0, 255, 255, 0, 0, 255, 0, 0, 0, 255,
                             0, 0, 0,   255, 0, 0,   0, 255, 0,   0, 0, 255, 0, 0, 0, 255};
            perspective.texture = texture;
            stats = render(Frame{{black, perspective}});
            require(stats.textured == 1 &&
                        pixel(24, 24) == std::array<std::uint8_t, 4>{0, 0, 255, 255},
                    "perspective-correct texture interpolation");
            auto projected = perspective;
            projected.vertices[1].position[3] = 1;
            projected.stage0_states[24] = 3 + 256;
            projected.texture_transform = kIdentity;
            projected.texture_transform[2] = 3;
            projected.texture_transform[14] = 1;
            stats = render(Frame{{black, projected}});
            require(pixel(24, 24) == std::array<std::uint8_t, 4>{0, 255, 0, 255},
                    "texture projection divides after interpolation");
            auto generated = triangle();
            generated.has_normals = true;
            generated.texture_coordinates = 0;
            generated.texture = texture;
            generated.stage0_states = perspective.stage0_states;
            generated.stage0_states[11] = 65536;
            generated.stage0_states[24] = 2;
            generated.texture_transform[0] = 0.5F;
            generated.texture_transform[12] = 0.5F;
            for (auto &vertex : generated.vertices)
                vertex.normal = {-1, 0, 0};
            stats = render(Frame{{black, generated}});
            require(stats.textured == 1 &&
                        pixel(16, 40) == std::array<std::uint8_t, 4>{0, 0, 255, 255},
                    "generated camera normals feed the texture matrix");
            auto fog = triangle();
            // W fog uses the current W-friendly projection even for XYZRHW vertices.
            fog.projection[11] = 1;
            fog.projection[15] = 0;
            fog.space = PositionSpace::Screen;
            fog.vertices[0].position = {8, 8, 0.5F, 1};
            fog.vertices[1].position = {56, 8, 0.5F, 1};
            fog.vertices[2].position = {8, 56, 0.5F, 1};
            fog.render_states[28] = 1;
            fog.render_states[35] = 3;
            fog.render_states[34] = 0xff0000ffU;
            fog.render_states[36] = 0;
            fog.render_states[37] = 0x40000000U;
            render(Frame{{black, fog}});
            require(pixel(24, 24) == std::array<std::uint8_t, 4>{128, 0, 128, 255}, "linear W fog");
            auto lit = triangle(0.25F, {1, 1, 1, 1});
            lit.has_normals = true;
            for (auto &vertex : lit.vertices)
                vertex.normal = {0, 0, -1};
            lit.render_states[137] = 1;
            lit.render_states[145] = 0;
            lit.world[10] = 2;
            Light light;
            light.type = LightType::Directional;
            light.direction = {0, 0, 1};
            light.diffuse = {0, 1, 0, 0};
            lit.lights.push_back(light);
            render(Frame{{black, lit}});
            require(pixel(16, 40) == std::array<std::uint8_t, 4>{0, 128, 0, 255},
                    "inverse-transpose normal under nonuniform scale");
            lit.render_states[143] = 1;
            render(Frame{{black, lit}});
            require(pixel(16, 40) == std::array<std::uint8_t, 4>{0, 255, 0, 255},
                    "normal normalization and directional lighting");
            auto unknown = triangle();
            unknown.render_states[151] = 1;
            stats = render(Frame{{black, unknown}});
            require(stats.draws == 0 && stats.unsupported.count("render state 151"),
                    "unimplemented vertex blending is counted");
            auto float_texture = std::make_shared<Texture>();
            float_texture->width = 1;
            float_texture->height = 1;
            float_texture->format = TextureFormat::Rgba32Float;
            float_texture->rgba = {1.0F / 31, 2.0F / 31, 3.0F / 31, 1};
            auto precise = perspective;
            precise.texture = float_texture;
            for (auto &vertex : precise.vertices) {
                vertex.position[3] = 1;
                vertex.uv[0] = {0, 0};
            }
            auto floating_descriptor =
                [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float
                                                                   width:64
                                                                  height:64
                                                               mipmapped:NO];
            floating_descriptor.storageMode = MTLStorageModeShared;
            floating_descriptor.usage = MTLTextureUsageRenderTarget;
            auto floating = [device newTextureWithDescriptor:floating_descriptor];
            pass.colorAttachments[0].texture = floating;
            render(Frame{{black, precise}});
            std::array<float, 4> exact{};
            [floating getBytes:exact.data()
                   bytesPerRow:16
                    fromRegion:MTLRegionMake2D(24, 24, 1, 1)
                   mipmapLevel:0];
            require(exact == std::array<float, 4>{1.0F / 31, 2.0F / 31, 3.0F / 31, 1},
                    "packed-color fractions retain FP32 precision before filtering");
            std::cout << "Metal device: native 3D transforms, depth, cull, alpha test, clipping, "
                         "ordered clears, large buffers and perspective textures passed on GPU; no "
                         "guest or asset inputs.\n";
            return 0;
        } catch (const std::exception &failure) {
            std::cerr << failure.what() << '\n';
            return 1;
        }
    }
}
