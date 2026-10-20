#include "platform/mac/hid_switch_controller.hpp"
#include "platform/mac/mac_audio.hpp"
#include "platform/mac/mac_clock.hpp"
#include "platform/mac/metal_device_frame.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_session.hpp"
#import <AppKit/AppKit.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

namespace {
struct Options {
    std::filesystem::path assets, controller_library, screenshot, capture_directory;
    std::set<std::uint32_t> capture_frames;
    std::uint32_t frames = 0;
    bool offscreen = false;
};
Options options(int argc, char **argv) {
    Options result;
    result.controller_library = std::filesystem::path(argv[0]).parent_path() /
                                "native-controller-deps/lib/libSDL3.0.dylib";
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--offscreen")
            result.offscreen = true;
        else if (index + 1 < argc && argument == "--assets")
            result.assets = argv[++index];
        else if (index + 1 < argc && argument == "--controller-library")
            result.controller_library = argv[++index];
        else if (index + 1 < argc && argument == "--screenshot")
            result.screenshot = argv[++index];
        else if (index + 1 < argc && argument == "--capture-directory")
            result.capture_directory = argv[++index];
        else if (index + 1 < argc && argument == "--capture-at") {
            std::istringstream list(argv[++index]);
            std::string item;
            while (std::getline(list, item, ','))
                result.capture_frames.insert(std::stoul(item));
        }
        else if (index + 1 < argc && argument == "--exit-after")
            result.frames = std::stoul(argv[++index]);
        else
            throw std::runtime_error("unknown or incomplete native option: " + argument);
    }
    if (result.assets.empty() || (result.offscreen && result.frames == 0))
        throw std::runtime_error(
            "usage: sadx-native --assets IMPORTED_ROOT [--offscreen --exit-after N] "
            "[--screenshot IMAGE.ppm] [--controller-library NATIVE_SDL3]");
    if (!result.offscreen && !result.screenshot.empty())
        throw std::runtime_error("native screenshot capture requires --offscreen");
    if (!result.capture_frames.empty() &&
        (!result.offscreen || result.capture_directory.empty() ||
         *result.capture_frames.begin() == 0 || *result.capture_frames.rbegin() > result.frames))
        throw std::runtime_error("native checkpoints require an offscreen frame range and directory");
    return result;
}

// NOTSA: platform application owner; all game updates run through the native session.
class Application {
  public:
    Options configuration;
    sadx::native::Display display;
    std::shared_ptr<const sadx::character_pack::CharacterPack> characters;
    sadx::input::mac::HidSwitchController controller;
    sadx::audio::mac::AudioOutput audio;
    sadx::MacPerformanceCounter clock;
    sadx::native::Session session;
    std::unique_ptr<sadx::MetalDeviceFrameRenderer> renderer;
    id<MTLCommandQueue> queue;
    std::uint32_t frames = 0;
    std::string failure;

    explicit Application(Options configuration, id<MTLDevice> device)
        : configuration(std::move(configuration)), display(this->configuration.assets / "system"),
          characters(std::make_shared<sadx::character_pack::CharacterPack>(
              sadx::character_pack::CharacterPack::load(
                  this->configuration.assets / "characters/sonic-startup.actions"))),
          session(display.output(), characters, display.frame_state(), display.arithmetic(),
                  [this](std::uint32_t port) {
                      if (port != 0)
                          throw std::runtime_error("native vibration port is unsupported");
                      if (!this->configuration.offscreen) {
                          std::string error;
                          const auto sample = controller.poll(error);
                          if (!error.empty())
                              throw std::runtime_error(error);
                          if (sample && sample->rumble_supported &&
                              !controller.rumble(0, 0, 0, error))
                              std::cerr << "native rumble stop failed: " << error << '\n';
                      }
                  }) {
        if (!device)
            throw std::runtime_error("Metal device unavailable");
        renderer = sadx::MetalDeviceFrameRenderer::create(device, &failure);
        if (!renderer)
            throw std::runtime_error(failure);
        queue = [device newCommandQueue];
        if (!queue)
            throw std::runtime_error("native Metal command queue unavailable");
        if (!this->configuration.offscreen) {
            if (!controller.initialize(this->configuration.controller_library.string(), failure))
                throw std::runtime_error(failure);
            if (!audio.start({}, failure))
                throw std::runtime_error(failure);
        }
    }
    void draw(MTLRenderPassDescriptor *pass, id<CAMetalDrawable> drawable = nil) {
        display.output().begin_frame();
        session.advance();
        auto command = [queue commandBuffer];
        const auto stats = renderer->encode(display.output().frame(), command, pass);
        if (!stats.unsupported.empty())
            throw std::runtime_error("native draw unsupported: " + stats.unsupported.begin()->first);
        if (drawable)
            [command presentDrawable:drawable];
        [command commit];
        [command waitUntilCompleted];
        if (command.status != MTLCommandBufferStatusCompleted)
            throw std::runtime_error("native Metal command failed");
        ++frames;
    }
    void report(std::ostream &output = std::cout) const {
        output << "{\"execution\":\"native\",\"frames\":" << frames
                  << ",\"game_mode\":" << static_cast<int>(session.flow_state().mode)
                  << ",\"game_state\":" << session.game_state().game_mode
                  << ",\"stage\":" << session.stage()
                  << ",\"rings\":" << session.rings()
                  << ",\"global_timer\":" << session.global_timer()
                  << ",\"logo_frames\":" << session.logo_scene().state().frames
                  << ",\"logo_mode\":" << static_cast<unsigned>(session.logo_scene().state().mode)
                  << ",\"textures\":" << display.output().loaded_textures() << "}\n";
    }
};

void save_image(id<MTLTexture> texture, const std::filesystem::path &path) {
    if (path.empty())
        return;
    std::vector<std::uint8_t> pixels(640 * 480 * 4);
    [texture getBytes:pixels.data() bytesPerRow:640 * 4
           fromRegion:MTLRegionMake2D(0, 0, 640, 480) mipmapLevel:0];
    std::ofstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("cannot write native screenshot");
    file << "P6\n640 480\n255\n";
    for (std::size_t offset = 0; offset < pixels.size(); offset += 4) {
        const char rgb[] = {char(pixels[offset + 2]), char(pixels[offset + 1]), char(pixels[offset])};
        file.write(rgb, 3);
    }
    if (!file)
        throw std::runtime_error("native screenshot write failed");
}

void run_offscreen(Application &app, id<MTLDevice> device) {
    auto color = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                  width:640 height:480 mipmapped:NO];
    color.storageMode = MTLStorageModeShared;
    color.usage = MTLTextureUsageRenderTarget;
    auto texture = [device newTextureWithDescriptor:color];
    auto depth = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                  width:640 height:480 mipmapped:NO];
    depth.storageMode = MTLStorageModePrivate;
    depth.usage = MTLTextureUsageRenderTarget;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = texture;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.depthAttachment.texture = [device newTextureWithDescriptor:depth];
    pass.depthAttachment.storeAction = MTLStoreActionStore;
    for (std::uint32_t frame = 0; frame < app.configuration.frames; ++frame) {
        pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass.depthAttachment.loadAction = MTLLoadActionClear;
        pass.depthAttachment.clearDepth = 1;
        app.draw(pass);
        if (app.configuration.capture_frames.count(app.frames)) {
            std::filesystem::create_directories(app.configuration.capture_directory);
            const auto prefix = app.configuration.capture_directory / std::to_string(app.frames);
            save_image(texture, prefix.string() + ".ppm");
            std::ofstream state(prefix.string() + ".json");
            if (!state)
                throw std::runtime_error("cannot write native checkpoint state");
            app.report(state);
        }
    }
    save_image(texture, app.configuration.screenshot);
}
} // namespace

@interface NativeView : MTKView <MTKViewDelegate>
@property(nonatomic, assign) Application *application;
@end
@implementation NativeView
- (void)drawInMTKView:(MTKView *)view {
    auto *app = self.application;
    auto pass = view.currentRenderPassDescriptor;
    auto drawable = view.currentDrawable;
    if (!pass || !drawable)
        return;
    try {
        app->draw(pass, drawable);
    } catch (const std::exception &error) {
        app->failure = error.what();
        self.paused = YES;
    }
}
- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    (void)view;
    (void)size;
}
@end

int main(int argc, char **argv) {
    @autoreleasepool {
        try {
            auto configuration = options(argc, argv);
            auto device = MTLCreateSystemDefaultDevice();
            Application app(std::move(configuration), device);
            if (app.configuration.offscreen) {
                run_offscreen(app, device);
            } else {
                [NSApplication sharedApplication];
                [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
                auto window = [[NSWindow alloc]
                    initWithContentRect:NSMakeRect(0, 0, 640, 480)
                              styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                backing:NSBackingStoreBuffered defer:NO];
                window.title = @"Sonic Adventure DX";
                auto view = [[NativeView alloc] initWithFrame:NSMakeRect(0, 0, 640, 480)
                                                      device:device];
                view.application = &app;
                view.delegate = view;
                view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
                view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
                view.drawableSize = CGSizeMake(640, 480);
                view.autoResizeDrawable = NO;
                // [platform:graphics] The pinned software limiter owns game cadence,
                // independently of the display's refresh rate.
                ((CAMetalLayer *)view.layer).displaySyncEnabled = NO;
                view.preferredFramesPerSecond = 63;
                view.paused = YES;
                view.enableSetNeedsDisplay = NO;
                window.contentView = view;
                [window center];
                [window makeKeyAndOrderFront:nil];
                [NSApp activateIgnoringOtherApps:YES];
                [NSApp finishLaunching];
                while (window.visible && app.failure.empty() &&
                       (!app.configuration.frames || app.frames < app.configuration.frames)) {
                    while (auto event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                         untilDate:NSDate.distantPast
                                                            inMode:NSDefaultRunLoopMode dequeue:YES])
                        [NSApp sendEvent:event];
                    [view draw];
                    const auto pacing = sadx::frame::run_speedrun_frame_limiter(
                        app.session.renderer_timing(), app.session.frame_mode(), 63,
                        app.clock.limiter_clock());
                    if (!pacing)
                        throw std::runtime_error("native profile frame limiter failed");
                }
            }
            app.report();
            if (!app.failure.empty())
                throw std::runtime_error(app.failure);
            return 0;
        } catch (const std::exception &error) {
            std::cerr << "native session stopped: " << error.what() << '\n';
            return 1;
        }
    }
}
