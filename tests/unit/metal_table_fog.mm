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
            const auto bits = [](float value) {
                std::uint32_t word;
                std::memcpy(&word, &value, 4);
                return word;
            };
            const auto base = [&](PositionSpace space) {
                auto draw = triangle(0.25F, {1, 0, 0, 128.0F / 255});
                draw.space = space;
                draw.render_states[28] = 1;
                draw.render_states[35] = 3;
                draw.render_states[34] = 0xff0000ffU;
                draw.render_states[36] = bits(0);
                draw.render_states[37] = bits(1);
                if (space == PositionSpace::Screen) {
                    draw.vertices[0].position = {8, 8, 0.25F, 1};
                    draw.vertices[1].position = {56, 8, 0.25F, 1};
                    draw.vertices[2].position = {8, 56, 0.25F, 1};
                } else {
                    draw.vertices[0].position = {-0.75F, 0.75F, 0.25F, 1};
                    draw.vertices[1].position = {0.75F, 0.75F, 0.25F, 1};
                    draw.vertices[2].position = {-0.75F, -0.75F, 0.25F, 1};
                }
                return draw;
            };
            const auto check = [&](const Draw &draw, double factor, const char *why) {
                auto stats = render(Frame{{black, draw}});
                require(stats.draws == 1 && stats.unsupported.empty(), why);
                const auto actual = pixel(24, 24);
                const std::array<long, 4> expected{std::lround(255 * (1 - factor)), 0,
                                                   std::lround(255 * factor), 128};
                for (unsigned channel = 0; channel < 4; ++channel)
                    require(std::abs(long(actual[channel]) - expected[channel]) <= 1, why);
            };
            for (auto space : {PositionSpace::Object, PositionSpace::Screen}) {
                auto draw = base(space);
                check(draw, 0.75, "affine Z fog, both vertex spaces, preserves opacity");
                draw.projection[10] = 2;
                // Object z becomes0.5; XYZRHW z remains0.25 despite this transform.
                check(draw, space == PositionSpace::Object ? 0.5 : 0.75,
                      "current projection determines depth source; screen geometry stays "
                      "pretransformed");
                draw.projection = kIdentity;
                draw.viewport.near_depth = 0.2F;
                draw.viewport.far_depth = 0.8F;
                check(draw, space == PositionSpace::Object ? 0.65 : 0.75,
                      "fog uses device Z after viewport mapping");
            }
            auto draw = base(PositionSpace::Screen);
            draw.vertices[0].position[2] = 0;
            draw.vertices[1].position[2] = 0.5F;
            draw.vertices[2].position[2] = 1;
            draw.vertices[1].position[3] = 0.5F;
            draw.vertices[2].position[3] = 0.25F;
            check(draw, 0.5, "device Z is interpolated in screen space despite varying RHW");
            draw.render_states[35] = 1;
            draw.render_states[38] = bits(1);
            check(draw, std::exp(-0.5), "exponential Z fog is evaluated per fragment");
            draw.render_states[35] = 2;
            check(draw, std::exp(-0.25), "exponential squared Z fog is evaluated per fragment");
            // At equal screen barycentrics, RHW=(1+1/2+1/4)/3=7/12, true W=12/7.
            for (auto space : {PositionSpace::Object, PositionSpace::Screen}) {
                draw = base(space);
                draw.projection[11] = 1;
                draw.projection[15] = 0;
                draw.projection[10] = 0.5F;
                const std::array<float, 3> w{1, 2, 4};
                for (unsigned index = 0; index < 3; ++index) {
                    if (space == PositionSpace::Screen) {
                        draw.vertices[index].position[3] = 1 / w[index];
                    } else {
                        draw.vertices[index].position[0] *= w[index];
                        draw.vertices[index].position[1] *= w[index];
                        draw.vertices[index].position[2] = w[index];
                    }
                }
                draw.render_states[37] = bits(4);
                draw.render_states[48] = 1;
                draw.render_states[140] = 99;
                check(draw, 4.0 / 7,
                      "W reconstructed after RHW interpolation; table overrides vertex/range fog");
                draw.render_states[35] = 1;
                draw.render_states[38] = bits(0.25F);
                check(draw, std::exp(-3.0 / 7), "exponential W fog after interpolation");
                draw.render_states[35] = 2;
                check(draw, std::exp(-9.0 / 49), "squared exponential W fog after interpolation");
            }
            draw = base(PositionSpace::Screen);
            draw.projection[3] = 1;
            auto stats = render(Frame{{black, draw}});
            require(stats.draws == 0 && stats.unsupported["table fog projection"] == 1,
                    "unsupported projection counted, not drawn");
            draw.render_states[28] = 0;
            check(draw, 1, "disabled fog ignores unsupported projection");
            draw = base(PositionSpace::Object);
            draw.render_states[37] = bits(0);
            stats = render(Frame{{black, draw}});
            require(stats.draws == 0 && stats.unsupported["zero fog interval"] == 1,
                    "zero interval counted");
            std::cout << "Metal table fog: affine Z, perspective W, both FVFs, viewport, formulas, "
                         "interpolation and unsupported counts pass\n";
            return 0;
        } catch (const std::exception &error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
}
