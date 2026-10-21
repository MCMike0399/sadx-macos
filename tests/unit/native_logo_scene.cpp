#include "runtime/logo_scene.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
struct Output final : sadx::native::LogoOutput {
    bool loaded = false, fail_load = false;
    unsigned loads = 0, releases = 0, texture_draws = 0, boxes = 0;
    std::vector<unsigned> order;
    void load_logo_archive(std::string_view name) override {
        assert(name == "SEGALOGO_E");
        if (fail_load)
            throw std::runtime_error("test asset failure");
        loaded = true;
        ++loads;
    }
    void select_logo_archive() override { assert(loaded); }
    void release_logo_archive() override {
        assert(loaded);
        loaded = false;
        ++releases;
    }
    void set_background(unsigned top, unsigned middle, unsigned bottom) override {
        assert(top == 0 && middle == 0 && bottom == 0);
    }
    void set_blending(unsigned side, unsigned mode) override {
        assert((side == 0 && mode == 8) || (side == 1 && mode == 6));
    }
    sadx::pvr::TextureInfo texture(unsigned index) override {
        assert(loaded && index < 18);
        return {index, 0, 256, 256};
    }
    void draw_texture(const std::array<sadx::pvr::Vertex, 4> &vertices,
                      unsigned index, unsigned alpha) override {
        assert(loaded && alpha == 1 && index < 18);
        assert(vertices[0].x < vertices[2].x && vertices[0].y < vertices[1].y);
        ++texture_draws;
        order.push_back(1);
    }
    void draw_polygon(const std::array<sadx::pvr::Point, 4> &points, unsigned,
                      float priority, unsigned flags) override {
        assert(points[0].x == 0 && points[3].x == 640 && points[3].y == 480);
        assert(priority == -11 && (flags == 34 || flags == 98));
        ++boxes;
        order.push_back(2);
    }
    void queue_box(float, float, float, float, float, unsigned) override {
        throw std::runtime_error("unexpected queued backdrop with fog emulation disabled");
    }
};
} // namespace

int main() {
    using namespace sadx;
    Output output;
    frame::ModeState mode;
    frame::RendererState renderer;
    renderer.counter_frequency = 6000;
    save::Data save;
    save.total_time = 123;
    std::int16_t rings = 7;
    float horizontal = 1, vertical = 1, queued_depth = 0;
    std::int32_t display_disabled = 0;
    std::uint8_t fog_emulation = 0, fog_enabled = 0;
    X87Context fp(127);
    native::LogoScene scene(output, mode, renderer, save, rings,
                            {horizontal, vertical, display_disabled, fog_emulation,
                             fog_enabled, queued_depth}, fp);
    ui_scale::State scaling(horizontal, vertical);
    scene.attach_ui_scaling(scaling);
    output.fail_load = true;
    bool failed = false;
    try {
        scene.initialize();
    } catch (const std::runtime_error &) {
        failed = true;
    }
    assert(failed && scene.state().mode == logo::Mode::Inactive);
    assert(save.total_time == 123 && rings == 7 && mode.logic_increment == 0);
    assert(scene.tasks() && scene.tasks()->source_free_slots() == 4096);

    output.fail_load = false;
    scene.initialize();
    assert(scene.tasks()->source_free_slots() == 4096);
    assert(mode.frame_multiplier == 1 && mode.logic_increment == 1);
    assert(renderer.frame_interval_multiplier == 1 && renderer.frame_interval == 100);
    assert(save.total_time == 0 && save.vibration == 1 && rings == 0);
    assert(scene.state().mode == logo::Mode::SegaHold && scene.fade_color() == 0);
    assert(scene.advance() == logo::Completion::Running);
    assert((output.order == std::vector<unsigned>{1, 1, 1, 1, 1, 1, 2}));
    assert(!scaling.enabled && scaling.entries.empty());

    // A live frame suppresses both immediate draw paths without suppressing logo logic.
    display_disabled = 1;
    scene.advance();
    assert(scene.state().frames == 2 && output.texture_draws == 6 && output.boxes == 1);
    display_disabled = 0;
    logo::Completion completion = logo::Completion::Running;
    unsigned steps = 0;
    while (completion == logo::Completion::Running && steps++ < 500)
        completion = scene.advance();
    assert(completion == logo::Completion::Movie);
    assert(output.releases == 1 && !output.loaded && scene.state().logic_entry_flag == 0);
    assert(scene.state().mode == logo::Mode::Inactive);
    scene.initialize();
    assert(output.loaded && output.loads == 2 && scene.tasks()->source_free_slots() == 4096);
    std::cout << "Native logo scene: asset failure prefix, task reset, profile save, frame control, "
                 "immediate draw order and release/re-entry pass; boot is separate.\n";
}
