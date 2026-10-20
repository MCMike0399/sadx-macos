#include "platform/mac/metal_device_frame.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_session.hpp"
#import <Foundation/Foundation.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace sadx;
void require(bool value, const std::string &message) {
    if (!value)
        throw std::runtime_error(message);
}
void run(int argc, char **argv) {
    require(argc == 4,
            "usage: mac-native-logo-scene-test SYSTEM_DIRECTORY CHARACTER_PACK OUTPUT.ppm");
    native::Display display(argv[1]);
    auto pack = std::make_shared<character_pack::CharacterPack>(character_pack::CharacterPack::load(argv[2]));
    unsigned vibration_stops = 0;
    native::Session session(display.output(), pack, display.frame_state(), display.arithmetic(),
                            [&](std::uint32_t port) {
                                require(port == 0, "startup vibration port");
                                ++vibration_stops;
                            });
    auto &output = display.output();
    require(session.flow_state().mode == flow::GameMode::Start, "native session begins at Start");
    // Seed distinct owners to expose aliasing hidden by fresh-session zero defaults.
    auto &initial_flow = const_cast<flow::GameFlowState &>(session.flow_state());
    initial_flow.title_mode = 7;
    initial_flow.tutorial_variant = 9;
    for (unsigned frame = 0; frame < 120; ++frame) {
        output.begin_frame();
        session.advance();
        if (frame == 0)
            require(session.flow_state().mode == flow::GameMode::LogoInit &&
                        session.global_timer() == 1 && session.game_state().game_mode == 21,
                    "first native frame follows original game startup before logo initialization");
        if (frame == 0)
            require(session.flow_state().title_mode == 7 &&
                        session.flow_state().tutorial_variant == 0,
                    "advertise initialization resets its selection owner and preserves title mode");
    }
    require(output.loaded_textures() == 18 && session.save_data().vibration == 1 &&
                vibration_stops == 1 && session.flow_state().mode == flow::GameMode::Logo,
            "native texture resources, profile save and flow composition");
    require(output.frame().commands.size() == 8, "clear, six tiles, immediate backdrop in source order");

    auto device = MTLCreateSystemDefaultDevice();
    require(device != nil, "Metal device unavailable");
    auto queue = [device newCommandQueue];
    std::string error;
    auto renderer = MetalDeviceFrameRenderer::create(device, &error);
    require(bool(renderer), error);
    auto descriptor =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                         width:640 height:480 mipmapped:NO];
    descriptor.storageMode = MTLStorageModeShared;
    descriptor.usage = MTLTextureUsageRenderTarget;
    auto target = [device newTextureWithDescriptor:descriptor];
    auto depth_descriptor =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                         width:640 height:480 mipmapped:NO];
    depth_descriptor.storageMode = MTLStorageModePrivate;
    depth_descriptor.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = target;
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.depthAttachment.texture = [device newTextureWithDescriptor:depth_descriptor];
    pass.depthAttachment.loadAction = MTLLoadActionClear;
    pass.depthAttachment.storeAction = MTLStoreActionStore;
    pass.depthAttachment.clearDepth = 1;
    auto command = [queue commandBuffer];
    const auto stats = renderer->encode(output.frame(), command, pass);
    for (const auto &[reason, count] : stats.unsupported)
        std::cerr << "native logo rejected " << count << " draws: " << reason << '\n';
    require(stats.unsupported.empty() && stats.draws == 7, "native logo draws accepted by shared device");
    [command commit];
    [command waitUntilCompleted];
    require(command.status == MTLCommandBufferStatusCompleted, "native logo GPU completion");
    std::vector<std::uint8_t> pixels(640 * 480 * 4);
    [target getBytes:pixels.data() bytesPerRow:640 * 4 fromRegion:MTLRegionMake2D(0, 0, 640, 480)
         mipmapLevel:0];
    std::ofstream image(argv[3], std::ios::binary);
    require(bool(image), "cannot create native logo image");
    image << "P6\n640 480\n255\n";
    for (std::size_t i = 0; i < pixels.size(); i += 4) {
        const char rgb[] = {char(pixels[i + 2]), char(pixels[i + 1]), char(pixels[i])};
        image.write(rgb, sizeof(rgb));
    }
    require(bool(image), "native logo image write failed");
    std::cout << "Native session: Start -> LogoInit -> Logo, 120 updates, imported character/texture "
                 "resources, source view and draw order, seven Metal draws. Later modes remain open.\n";
}
} // namespace

int main(int argc, char **argv) {
    @autoreleasepool {
        try {
            run(argc, argv);
            return 0;
        } catch (const std::exception &error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
}
