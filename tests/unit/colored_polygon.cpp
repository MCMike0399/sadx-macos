#include "engine/pvr_device.hpp"
#include "engine/native_texture_lifetime.hpp"
#include <cassert>
#include <cstring>

int main() {
    using namespace sadx;
    graphics::Draw state;
    state.viewport = {17, 31, 640, 480, 0, 1};
    state.render_states[7] = 6;
    state.render_states[22] = 7;
    state.render_states[137] = 8;
    state.render_states[145] = 9;
    state.has_diffuse = false;
    state.stage0_states[16] = 2;
    state.texture = std::make_shared<graphics::Texture>();
    graphics::Frame frame;
    std::vector<texture_lifetime::Memory> pool(1);
    std::int32_t cached = 123;
    auto *active = &pool[0];
    texture_lifetime::List list;
    const auto *selected = &list;
    std::array<std::uint32_t, 8> temporary{};
    texture_lifetime::State textures{pool, cached, active, selected, temporary};
    unsigned clears = 0, projections = 0;
    texture_lifetime::Services services{[&] {
        assert(state.render_states.at(27) == 1);
        assert(state.render_states.at(22) == 7 && state.render_states.at(137) == 8 &&
               state.render_states.at(145) == 9);
        ++clears;
        state.texture.reset();
    }};
    pvr::ImmediateDevice device{state, frame, [&](float reciprocal) {
        assert(reciprocal == 0.25f && state.stage0_states.at(16) == 1);
        assert(state.render_states.at(22) == 1 && state.render_states.at(137) == 0 &&
               state.render_states.at(145) == 1);
        assert(cached == -1 && active == nullptr);
        ++projections;
        return 0.75f;
    }};
    const std::array<pvr::Point, 4> points{{{-4, 7}, {-4, 47}, {64, 7}, {64, 47}}};
    const std::array<std::uint32_t, 4> colors{
        4294901760U, 4278255360U, 4278190335U, 2155905152U};
    X87Context fp(127);
    pvr::submit_colored_polygon(points, colors, -4, 96, device, fp, [&] {
        texture_lifetime::unbind_cached_texture(textures, services);
    });
    assert(frame.commands.size() == 1 && clears == 1 && projections == 4);
    assert(selected == &list && cached == -1 && active == nullptr);
    const auto &draw = std::get<graphics::Draw>(frame.commands.front());
    assert(!draw.texture && draw.primitive_type == 5 && draw.primitive_count == 2);
    assert(draw.viewport.x == 17 && draw.render_states.at(7) == 6);
    assert(draw.texture_coordinates == 0 && draw.has_diffuse && !draw.has_specular && !draw.has_normals);
    assert(draw.vertices.size() == 4 && draw.stage0_states.at(16) == 1);
    assert(state.stage0_states.at(16) == 2);
    for (unsigned index = 0; index < 4; ++index) {
        const auto &v = draw.vertices[index];
        assert(v.position[0] == points[index].x + 0.5f);
        assert(v.position[1] == points[index].y + 0.5f);
        assert(v.position[2] == 0.75f && v.position[3] == 1);
        const auto color = colors[index];
        const std::array<float, 4> expected{
            float((color >> 16) & 255) / 255, float((color >> 8) & 255) / 255,
            float(color & 255) / 255, float(color >> 24) / 255};
        assert(v.diffuse == expected);
    }
    // Reciprocal conversion faults before the clamp or any device callbacks.
    const std::uint32_t negative_tiny_bits = 2147483649U;
    float negative_tiny;
    std::memcpy(&negative_tiny, &negative_tiny_bits, sizeof(negative_tiny));
    X87Context overflow_unmasked(119);
    bool overflow = false;
    try {
        pvr::submit_colored_polygon(points, colors, negative_tiny, 96, device,
                                    overflow_unmasked, [&] { assert(false); });
    } catch (const X87Error &) {
        overflow = true;
    }
    assert(overflow && projections == 4 && frame.commands.size() == 1);
}
