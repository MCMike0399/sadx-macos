#include "game/logo.hpp"
#include "engine/ninja_texture.hpp"
#include "platform/mac/metal_device_frame.hpp"
#import <Foundation/Foundation.h>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace sadx;
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
std::vector<std::uint8_t> bytes(const char *path) {
    std::ifstream input(path, std::ios::binary);
    require(bool(input), "missing caller-supplied logo PVM");
    return {std::istreambuf_iterator<char>(input), {}};
}
// This direct-entry renderer fixture uses explicit screen-space device defaults.
// It is not the unported ghDrawPvrTexture/queued-box implementation or native boot.
struct Display final : logo::DisplayCalls {
    const NinjaTextureArchive &archive;
    std::uint32_t &first_color;
    graphics::Frame tiles;
    std::vector<std::shared_ptr<const graphics::Texture>> textures;
    std::uint32_t base = 4294967295U;
    explicit Display(const NinjaTextureArchive &archive, std::uint32_t &color)
        : archive(archive), first_color(color) {}
    void select_logo_textures() override {}
    void set_material(std::uint32_t color) override {
        require(color == 4294967295U && base == 4294967295U,
                "demo enters the source ghInitPvrTexture white material state");
        first_color = color;
    }
    void default_blending() override {}
    void set_base_color(std::uint32_t color) override {
        base = color;
        first_color = color;
    }
    graphics::Draw quad(float x, float y, float width, float height, std::uint32_t color) {
        graphics::Draw draw;
        draw.space = graphics::PositionSpace::Screen;
        draw.primitive_type = 4;
        draw.primitive_count = 2;
        draw.viewport = {0, 0, 640, 480, 0, 1};
        draw.indices = {0, 1, 2, 2, 1, 3};
        draw.render_states = {{7, 0}, {15, 0}, {19, 5}, {20, 6}, {22, 1},
                              {27, 1}, {28, 0}, {137, 0}, {171, 1}};
        draw.stage0_states = {{1, 4}, {2, 2}, {3, 0}, {4, 4}, {5, 2}, {6, 0},
                              {13, 1}, {14, 1}, {16, 1}, {17, 1}, {18, 0}};
        for (unsigned i = 0; i < 4; ++i) {
            graphics::Vertex vertex;
            vertex.position = {x + (i >= 2 ? width : 0), y + (i % 2 ? height : 0), 0, 1};
            vertex.diffuse = {float((color >> 16) & 255) / 255,
                              float((color >> 8) & 255) / 255, float(color & 255) / 255,
                              float(color >> 24) / 255};
            vertex.uv[0] = {i >= 2 ? 1.0f : 0.0f, i % 2 ? 1.0f : 0.0f};
            draw.vertices.push_back(vertex);
        }
        return draw;
    }
    void draw_texture(std::uint32_t texture, float x, float y, float depth) override {
        require(depth == 1.1f, "source logo tile depth");
        auto decoded = decode_ninja_texture(archive, texture);
        require(bool(decoded) && !decoded.levels.empty(), "logo texture decode failed");
        const auto &level = decoded.levels.front();
        auto owned = std::make_shared<graphics::Texture>();
        owned->width = level.width;
        owned->height = level.height;
        owned->bgra = level.rgba8;
        for (std::size_t i = 0; i < owned->bgra.size(); i += 4)
            std::swap(owned->bgra[i], owned->bgra[i + 2]);
        auto draw = quad(x, y, level.width, level.height, base);
        draw.texture_coordinates = 1;
        draw.texture = owned;
        textures.push_back(owned);
        tiles.commands.emplace_back(std::move(draw));
    }
    void draw_box(float width, float height, float depth, std::uint32_t color) override {
        require(width == 640 && height == 480 && depth == -11.0f,
                "source logo backdrop call");
        // Explicit demo painter order models the known background role; production
        // queue sorting/depth behavior remains outside this fixture's claim.
        auto draw = quad(0, 0, width, height, color);
        draw.stage0_states[1] = 2;
        draw.stage0_states[2] = 0;
        draw.stage0_states[4] = 2;
        draw.stage0_states[5] = 0;
        tiles.commands.insert(tiles.commands.begin(), std::move(draw));
    }
};
void run(int argc, char **argv) {
    require(argc == 3, "usage: mac-logo-demo-test SEGALOGO_E.PVM OUTPUT.ppm");
    auto parsed = parse_ninja_texture_resource(bytes(argv[1]));
    require(bool(parsed), "logo PVM parse failed");
    auto device = MTLCreateSystemDefaultDevice();
    require(device != nil, "Metal device unavailable");
    auto queue = [device newCommandQueue];
    std::string error;
    auto renderer = MetalDeviceFrameRenderer::create(device, &error);
    require(bool(renderer), error.c_str());
    std::int32_t frames = 120;
    float horizontal = 1, vertical = 1;
    std::uint32_t color = 4294967295U;
    logo::DisplayState state{frames, horizontal, vertical, color};
    Display display(parsed.archive, color);
    X87Context fp(127);
    logo::display_profile_640(0, state, display, fp, 640, 480);
    require(display.tiles.commands.size() == 7, "six source tiles and backdrop");
    bool rejected = false;
    try {
        logo::display_profile_640(0, state, display, fp, 1280, 720);
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    require(rejected, "uncomposed profile viewport remains explicit");
    horizontal = 1.25f;
    rejected = false;
    try {
        logo::display_profile_640(0, state, display, fp, 640, 480);
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    horizontal = 1;
    require(rejected && display.tiles.commands.size() == 7,
            "uncomposed profile stretch stops before any native draw");
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                      width:640 height:480
                                                                   mipmapped:NO];
    descriptor.storageMode = MTLStorageModeShared;
    descriptor.usage = MTLTextureUsageRenderTarget;
    auto target = [device newTextureWithDescriptor:descriptor];
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = target;
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
    auto command = [queue commandBuffer];
    const auto stats = renderer->encode(display.tiles, command, pass);
    require(stats.unsupported.empty() && stats.draws == 7, "shared renderer accepts native demo");
    [command commit];
    [command waitUntilCompleted];
    require(command.status == MTLCommandBufferStatusCompleted, "GPU completion failed");
    std::vector<std::uint8_t> pixels(640 * 480 * 4);
    [target getBytes:pixels.data() bytesPerRow:640 * 4 fromRegion:MTLRegionMake2D(0, 0, 640, 480)
         mipmapLevel:0];
    std::ofstream output(argv[2], std::ios::binary);
    output << "P6\n640 480\n255\n";
    unsigned white = 0, blue = 0;
    for (std::size_t i = 0; i < pixels.size(); i += 4) {
        const char rgb[] = {char(pixels[i + 2]), char(pixels[i + 1]), char(pixels[i])};
        output.write(rgb, 3);
        white += pixels[i] > 240 && pixels[i + 1] > 240 && pixels[i + 2] > 240;
        blue += pixels[i] > pixels[i + 2] + 60 && pixels[i] > 120;
    }
    require(white > 640 * 480 * 0.8 && blue > 2000, "native asset demo has white/blue SEGA frame");
    std::cout << "Direct-entry native logo asset demo: 6 source tiles, shared Metal renderer; "
                 "640x480 only. Boot, task/draw composition and N1 remain unverified.\n";
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
