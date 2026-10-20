#include "engine/pvr_device.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>

int main() {
    using namespace sadx;
    pvr::State pvr;
    pvr.vertices[0].x = 42;
    pvr::initialize(pvr);
    assert(pvr.vertices[0].x == 42 && pvr.base_color == 4294967295U);
    pvr::set_material(4294967295U, pvr);
    pvr::set_base_color(2151686160U, pvr);
    for (const auto &v : pvr.vertices)
        assert(v.color == 2151686160U);
    assert(pvr.alpha_enabled == 1);
    std::vector<std::array<std::uint32_t, 2>> blends;
    pvr::default_blending([&](auto selector, auto value) {
        blends.push_back({selector, value});
    });
    assert((blends == std::vector<std::array<std::uint32_t, 2>>{{0, 8}, {1, 6}}));
    graphics::Draw inherited;
    inherited.viewport = {17, 31, 640, 480, 0, 1};
    inherited.render_states[7] = 1;
    inherited.stage0_states[1] = 4;
    graphics::Frame frame;
    std::vector<float> depths;
    pvr::ImmediateDevice device{inherited, frame, [&](float w) {
        depths.push_back(w);
        return w * 0.25f;
    }};
    X87Context fp(127);
    auto texture = std::make_shared<graphics::Texture>();
    texture->width = texture->height = 1;
    pvr.vertices[0].reciprocal_depth = 2;
    pvr::submit_texture(pvr.vertices, texture, 1, true, device, fp);
    assert(depths.size() == 4 && depths[0] == 2);
    const auto &draw = std::get<graphics::Draw>(frame.commands[0]);
    assert(draw.vertices[0].position[0] == 42.5f);
    assert(draw.vertices[0].position[2] == 0.5f);
    assert(draw.render_states.at(7) == 1 && draw.viewport.x == 17);
    assert(draw.stage0_states.at(13) == 3 && draw.stage0_states.at(14) == 3);
    assert(draw.stage0_states.at(16) == 1 && inherited.stage0_states.at(16) == 2);
    assert(draw.render_states.at(28) == 0 && inherited.render_states.at(28) == 1);
    assert(draw.primitive_type == 5 && draw.primitive_count == 2);
    const std::array<pvr::Point, 4> points{{{0, 0}, {0, 480}, {640, 0}, {640, 480}}};
    pvr::submit_box(points, 4294967295U, -11, 34, device, fp);
    const auto &box = std::get<graphics::Draw>(frame.commands[1]);
    assert(!box.texture && box.texture_coordinates == 0 && box.vertices.size() == 4);
    assert(box.render_states.at(27) == 0);
    assert(depths.size() == 8 && depths[4] > 0.09f && depths[4] < 0.1f);
    device.projected_depth = {};
    bool rejected = false;
    try {
        pvr::submit_box(points, 0, -1, 98, device, fp);
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    assert(rejected && frame.commands.size() == 2);
    struct TextureCalls final : pvr::Calls {
        std::uint32_t width = 256;
        bool drawn = false;
        pvr::TextureInfo texture(std::uint32_t) override { return {123, 0, width, 128}; }
        void draw_texture(const std::array<pvr::Vertex, 4> &, std::uint32_t,
                          std::uint32_t) override { drawn = true; }
        void draw_polygon(const std::array<pvr::Point, 4> &, std::uint32_t,
                          float, std::uint32_t) override { assert(false); }
        void queue_box(float, float, float, float, float, std::uint32_t) override {
            assert(false);
        }
    } texture_calls;
    pvr::State prefix;
    pvr::initialize(prefix);
    for (auto &v : prefix.vertices) {
        v.x = 17;
        v.y = 31;
        v.reciprocal_depth = 5;
    }
    const std::uint32_t tiny_bits = 1;
    float tiny;
    std::memcpy(&tiny, &tiny_bits, sizeof(tiny));
    X87Context unmasked_overflow(119);
    bool overflow = false;
    try {
        pvr::draw_texture(0, 3, 7, tiny, prefix, 0, texture_calls, unmasked_overflow);
    } catch (const X87Error &) {
        overflow = true;
    }
    // The exact reciprocal fits x87; its first binary32 store overflows after these stores.
    assert(overflow && prefix.vertices[1].y == 135);
    assert(prefix.vertices[2].x == 259 && prefix.vertices[3].x == 259);
    assert(prefix.vertices[0].x == 17 && prefix.vertices[0].y == 31);
    assert(prefix.vertices[0].reciprocal_depth == 5 && !texture_calls.drawn);
    texture_calls.width = 2147483649U;
    pvr::State centered;
    pvr::initialize(centered);
    X87Context unmasked_precision(95);
    pvr::draw_texture(16777216U, 0, 0, 1, centered, 0, texture_calls, unmasked_precision);
    assert(texture_calls.drawn && centered.vertices[0].x == -1073741824.0f);
    assert(centered.vertices[2].x == 1073741824.0f);
    std::cout << "PVR source initialization, colors, state inheritance and immediate strip submission pass\n";
}
