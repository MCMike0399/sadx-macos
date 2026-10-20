#include "platform/mac/hid_switch_controller.hpp"
#include "platform/mac/mac_audio.hpp"
#include "platform/mac/mac_clock.hpp"
#include "platform/mac/metal_device_frame.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_session.hpp"
#include "runtime/movie_scene.hpp"
#include "platform/mac/mac_movie.hpp"
#include "apps/native_assets.hpp"
#import <AppKit/AppKit.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <map>
#include <cstring>

namespace {
void save_image(id<MTLTexture>, const std::filesystem::path &);
struct Options {
    struct KeyEvent { std::uint32_t key; bool down; };
    std::filesystem::path assets, controller_library, screenshot, capture_directory;
    std::set<std::uint32_t> capture_frames;
    std::uint32_t frames = 0;
    bool offscreen = false;
    std::multimap<std::uint32_t, KeyEvent> keys;
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
        else if (index + 1 < argc && argument == "--key-events") {
            std::istringstream list(argv[++index]);
            std::string item;
            while (std::getline(list, item, ',')) {
                std::istringstream event(item);
                std::string frame, key, action;
                if (!std::getline(event, frame, ':') || !std::getline(event, key, ':') ||
                    !std::getline(event, action) || (action != "down" && action != "up"))
                    throw std::runtime_error("key events use FRAME:WINDOWS_KEY:down|up");
                result.keys.emplace(std::stoul(frame),
                                    Options::KeyEvent{static_cast<std::uint32_t>(std::stoul(key)),
                                                     action == "down"});
            }
        }
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
    sadx::native::app::ImportedAssets assets;
    sadx::native::Display display;
    std::shared_ptr<const sadx::character_pack::CharacterPack> characters;
    sadx::input::mac::HidSwitchController controller;
    sadx::audio::mac::AudioOutput audio;
    sadx::MacPerformanceCounter clock;
    sadx::native::Session session;
    std::unique_ptr<sadx::native::Input> input;
    std::unique_ptr<sadx::native::MovieScene> movie;
    std::unique_ptr<sadx::movie::mac::MoviePlayer> media;
    sadx::movie::mac::MovieUpdate media_update;
    sadx::audio::mac::AudioStats last_movie_audio{};
    std::function<void()> present_target;
    std::unique_ptr<sadx::MetalDeviceFrameRenderer> renderer;
    id<MTLCommandQueue> queue;
    std::uint32_t frames = 0;
    std::string failure;
    std::uint32_t key_event_frame = 0, decoded_frames = 0;
    std::uint32_t controller_instance = 0;
    bool focused = true;
    NSWindow *window = nil;
    std::int32_t pending_movie_volume = 0;

    explicit Application(Options configuration, id<MTLDevice> device)
        : configuration(std::move(configuration)),
          assets(sadx::native::app::load_assets(this->configuration.assets)),
          display(this->configuration.assets / "system"),
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
        input = sadx::native::Input::create(assets.input, display.sse(), failure);
        if (!input)
            throw std::runtime_error(failure);
        configure_movie();
    }
    void encode(MTLRenderPassDescriptor *pass, id<CAMetalDrawable> drawable = nil) {
        @autoreleasepool {
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
            if (configuration.frames && frames >= configuration.frames)
                session.request_quit();
        }
    }
    void advance() {
        pump_events();
        poll_devices();
        display.output().begin_frame();
        session.advance();
        present_target();
        session.complete_frame();
    }
    void pace() {
        const auto result = sadx::frame::run_speedrun_frame_limiter(
            session.renderer_timing(), session.frame_mode(), 63, clock.limiter_clock());
        if (!result)
            throw std::runtime_error("native profile frame limiter failed");
    }
    void pump_events();
    void poll_devices() {
        std::string error;
        std::optional<sadx::InputModGamepadState> sample;
        if (!configuration.offscreen) {
            const auto connected = controller.poll(error);
            if (!error.empty())
                throw std::runtime_error(error);
            if (connected) {
                if (controller_instance != 0 && controller_instance != connected->instance_id)
                    input->gamepad(0, std::nullopt);
                controller_instance = connected->instance_id;
                sample = connected->state;
            } else {
                controller_instance = 0;
            }
        }
        input->gamepad(0, sample);
        if (!input->poll_frame(session.input_frame_context(), session.soft_reset_state(), error))
            throw std::runtime_error(error);
    }
    void update_controllers() {
        std::string error;
        // No gameplay camera task exists during native startup; renderer View is a different owner.
        if (!input->update_controllers(static_cast<std::uint16_t>(session.stage()),
                                       sadx::PlayerInputCamera{false, {}},
                                       sadx::native::controller_constants(),
                                       display.arithmetic(), error))
            throw std::runtime_error(error);
    }
    void configure_movie();
    void capture(id<MTLTexture> texture) {
        if (configuration.capture_frames.count(frames)) {
            std::filesystem::create_directories(configuration.capture_directory);
            const auto prefix = configuration.capture_directory / std::to_string(frames);
            save_image(texture, prefix.string() + ".ppm");
            std::ofstream state(prefix.string() + ".json");
            if (!state)
                throw std::runtime_error("cannot write native checkpoint state");
            report(state);
        }
    }
    void report(std::ostream &output = std::cout) const {
        const auto movie_audio = media ? media->audio_stats() : last_movie_audio;
        output << "{\"execution\":\"native\",\"frames\":" << frames
                  << ",\"game_mode\":" << static_cast<int>(session.flow_state().mode)
                  << ",\"game_state\":" << session.game_state().game_mode
                  << ",\"stage\":" << session.stage()
                  << ",\"rings\":" << session.rings()
                  << ",\"global_timer\":" << session.global_timer()
                  << ",\"logo_frames\":" << session.logo_scene().state().frames
                  << ",\"logo_mode\":" << static_cast<unsigned>(session.logo_scene().state().mode)
                  << ",\"textures\":" << display.output().loaded_textures()
                  << ",\"completed_frames\":" << session.completed_frames()
                  << ",\"input_held\":" << (input ? input->held(0) : 0)
                  << ",\"movie_decoded_frames\":" << decoded_frames
                  << ",\"movie_textures\":" << movie->loaded_textures()
                  << ",\"movie_media_open\":" << (media ? "true" : "false")
                  << ",\"movie_phase\":" << static_cast<unsigned>(movie->state().phase)
                  << ",\"movie_source_padding_defined\":false"
                  << ",\"movie_audio_frames\":" << movie_audio.submitted_frames
                  << ",\"movie_audio_nonzero_samples\":" << movie_audio.nonzero_samples
                  << ",\"movie_audio_completed_buffers\":" << movie_audio.completed_buffers
                  << ",\"movie_audio_device_error\":" << movie_audio.device_error
                  << "}\n";
    }
};

// [platform:input] Carbon physical key codes to source Windows virtual keys (US layout).
std::uint32_t windows_key(unsigned short code) {
    static const std::map<unsigned short, std::uint32_t> keys{
        {0x00,'A'}, {0x0b,'B'}, {0x08,'C'}, {0x02,'D'}, {0x0e,'E'}, {0x03,'F'},
        {0x05,'G'}, {0x04,'H'}, {0x22,'I'}, {0x26,'J'}, {0x28,'K'}, {0x25,'L'},
        {0x2e,'M'}, {0x2d,'N'}, {0x1f,'O'}, {0x23,'P'}, {0x0c,'Q'}, {0x0f,'R'},
        {0x01,'S'}, {0x11,'T'}, {0x20,'U'}, {0x09,'V'}, {0x0d,'W'}, {0x07,'X'},
        {0x10,'Y'}, {0x06,'Z'}, {0x1d,'0'}, {0x12,'1'}, {0x13,'2'}, {0x14,'3'},
        {0x15,'4'}, {0x17,'5'}, {0x16,'6'}, {0x1a,'7'}, {0x1c,'8'}, {0x19,'9'},
        {0x24,13}, {0x4c,13}, {0x35,27}, {0x31,32}, {0x33,8}, {0x30,9},
        {0x7b,37}, {0x7e,38}, {0x7c,39}, {0x7d,40},
        {0x38,160}, {0x3c,161}, {0x3b,162}, {0x3e,163}, {0x3a,164}, {0x3d,165},
        {0x37,91}, {0x36,92}, {0x7a,112}, {0x78,113}, {0x63,114}, {0x76,115},
        {0x60,116}, {0x61,117}, {0x62,118}, {0x64,119}, {0x65,120}, {0x6d,121},
        {0x67,122}, {0x6f,123}, {0x73,36}, {0x77,35}, {0x74,33}, {0x79,34},
        {0x52,96}, {0x53,97}, {0x54,98}, {0x55,99}, {0x56,100}, {0x57,101},
        {0x58,102}, {0x59,103}, {0x5b,104}, {0x5c,105}, {0x47,12}, {0x39,20},
        {0x75,46}, {0x29,186}, {0x18,187}, {0x2b,188}, {0x1b,189}, {0x2f,190},
        {0x2c,191}, {0x32,192}, {0x21,219}, {0x2a,220}, {0x1e,221}, {0x27,222}};
    const auto found = keys.find(code);
    return found == keys.end() ? 0 : found->second;
}
void Application::pump_events() {
    if (!configuration.offscreen) {
        const bool active = NSApp.active;
        if (focused && !active)
            input->focus_lost();
        focused = active;
        while (auto event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:NSDate.distantPast
                                                inMode:NSDefaultRunLoopMode dequeue:YES]) {
            if (focused && (event.type == NSEventTypeKeyDown || event.type == NSEventTypeKeyUp)) {
                if (const auto key = windows_key(event.keyCode))
                    input->key_event(key, event.type == NSEventTypeKeyDown);
            } else if (focused && event.type == NSEventTypeFlagsChanged) {
                if (const auto key = windows_key(event.keyCode))
                    input->key_event(key, CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState,
                                                               event.keyCode));
            }
            [NSApp sendEvent:event];
        }
        if (window && !window.visible)
            session.request_quit();
    }
    while (key_event_frame < frames + 1) {
        ++key_event_frame;
        const auto range = configuration.keys.equal_range(key_event_frame);
        for (auto event = range.first; event != range.second; ++event)
            input->key_event(event->second.key, event->second.down);
    }
}
void Application::configure_movie() {
    sadx::native::MovieHost host;
    host.present = [this] {
        present_target();
        display.output().begin_frame();
    };
    host.enable_pad_reading = [this] { input->enable_pad_reading(); };
    host.poll_input = [this] { update_controllers(); };
    host.pump_messages = [this] {
        pump_events();
        poll_devices();
    };
    host.open_clip = [this](const std::string &name, std::int32_t volume) {
        if (media) {
            media->stop();
            last_movie_audio = media->audio_stats();
        }
        std::string error;
        media = sadx::movie::mac::MoviePlayer::open(assets.movie_files.at(name).string(), error);
        if (!media)
            throw std::runtime_error("native movie open failed: " + error);
        media_update = {};
        pending_movie_volume = volume;
        return true;
    };
    host.start_clip = [this] {
        std::string error;
        if (!media || !media->start(pending_movie_volume, {}, error))
            throw std::runtime_error("native movie start failed: " + error);
    };
    host.close_clip = [this] {
        if (media) {
            media->stop();
            last_movie_audio = media->audio_stats();
            media.reset();
        }
        media_update.ready_frame = nullptr;
    };
    host.clip_running = [this] {
        if (!media)
            throw std::runtime_error("native movie has no open media owner");
        media_update = media->update();
        if (!media_update)
            throw std::runtime_error(media_update.error);
        return !media_update.complete;
    };
    host.decoded_frame = [this]() -> std::optional<sadx::native::MovieFrameView> {
        if (!media_update.ready_frame)
            return std::nullopt;
        const auto &frame = *media_update.ready_frame;
        ++decoded_frames;
        return sadx::native::MovieFrameView{frame.width, frame.height,
                                           frame.bgra.data(), frame.bgra.size()};
    };
    host.milliseconds = [this] { return static_cast<std::uint32_t>(clock.now() / 10000); };
    host.microseconds = [this] {
        const auto product = static_cast<std::uint64_t>(clock.now()) * 1000000U;
        std::int64_t signed_product;
        std::memcpy(&signed_product, &product, sizeof(product));
        return static_cast<std::uint32_t>(signed_product / sadx::MacPerformanceCounter::frequency_hz);
    };
    host.soft_reset = [this] {
        return static_cast<std::int32_t>(sadx::native::consume_soft_reset(session.soft_reset_state()));
    };
    host.reset_input_mask = [this] { return input->switch_data(); };
    host.reset_key = [this](std::uint32_t key) { return input->check_key(key); };
    host.held_buttons = [this] { return input->held(0); };
    host.clear_game = [] { throw sadx::flow::UnrecoveredGameFlowCall("clear_game"); };
    host.reset_game = [] { throw sadx::flow::UnrecoveredGameFlowCall("reset_game"); };
    movie = std::make_unique<sadx::native::MovieScene>(
        display, session.frame_mode(), session.renderer_timing(), session.movie_state(),
        assets.movies, assets.music_volume, std::move(host));
    session.attach_movie(*movie);
}

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
    app.present_target = [&app, pass, texture] {
        pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass.depthAttachment.loadAction = MTLLoadActionClear;
        pass.depthAttachment.clearDepth = 1;
        app.encode(pass);
        app.capture(texture);
        if (app.session.flow_state().mode == sadx::flow::GameMode::Movie)
            app.pace();
    };
    try {
        while (!app.session.quit_requested() && app.frames < app.configuration.frames)
            app.advance();
    } catch (...) {
        save_image(texture, app.configuration.screenshot);
        app.report();
        throw;
    }
    save_image(texture, app.configuration.screenshot);
}
} // namespace

@interface NativeView : MTKView
@end
@implementation NativeView
- (BOOL)acceptsFirstResponder { return YES; }
// The application event pump already delivered these events to the native input owner.
- (void)keyDown:(NSEvent *)event { (void)event; }
- (void)keyUp:(NSEvent *)event { (void)event; }
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
                app.window = window;
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
                [window makeFirstResponder:view];
                [NSApp activateIgnoringOtherApps:YES];
                [NSApp finishLaunching];
                app.present_target = [&app, view] {
                    @autoreleasepool {
                        if (!app.window.visible)
                            return;
                        auto pass = view.currentRenderPassDescriptor;
                        auto drawable = view.currentDrawable;
                        if (!pass || !drawable)
                            throw std::runtime_error("native window drawable unavailable");
                        app.encode(pass, drawable);
                        [view releaseDrawables];
                        app.pace();
                    }
                };
                while (window.visible && !app.session.quit_requested() &&
                       (!app.configuration.frames || app.frames < app.configuration.frames)) {
                    app.advance();
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
