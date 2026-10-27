#include "player/gamma_cursors.hpp"
#include <cstring>
#include <iostream>

namespace {
using namespace sadx;
using namespace sadx::gamma_cursors;
void require(bool result, const char *message) {
    if (!result) throw std::runtime_error(message);
}
std::uint32_t word(TaskWorkArena &arena, const TaskWorkHandle &buffer, unsigned index) {
    std::uint32_t value = 0;
    require(bool(arena.read(buffer, index * 4, &value, 4)), "read expanded word");
    return value;
}
void run() {
    TaskWorkArena arena;
    const std::vector<std::uint32_t> points{0x80000000, 0x7f800001, 0x7fc00002,
                                           0xdeadbeef, 0x01234567, 0x3f800001};
    const std::vector<std::uint32_t> colors{0xaabbccdd, 0x12345678};
    const std::vector<std::int16_t> indices{0, -1};
    WireModel model{};
    model.points = {&points, 3};
    model.colors = {&colors, 1};
    model.indices = &indices;
    model.line_count = 1;
    expand(model, arena);
    require(model.expanded_points->requested_size() == 24 &&
        model.expanded_colors->requested_size() == 8 && model.point_count == 2 && !model.texture,
        "real source allocation extents and output stores");
    for (unsigned component = 0; component < 3; ++component) {
        require(word(arena, model.expanded_points, component) == points[3 + component],
                "first indexed vertex raw word");
        require(word(arena, model.expanded_points, 3 + component) == points[component],
                "signed negative index selects preceding point");
    }
    require(word(arena, model.expanded_colors, 0) == colors[1] &&
        word(arena, model.expanded_colors, 1) == colors[0], "indexed source color words");
    const auto old_points = model.expanded_points;
    const auto old_colors = model.expanded_colors;
    expand(model, arena);
    require(!old_points->released() && !old_colors->released() &&
        old_points != model.expanded_points && old_colors != model.expanded_colors,
        "original overwritten buffers remain allocated");
    Templates templates;
    templates.first = model;
    templates.second = model;
    templates.second.line_count = 0;
    WireModel first{}, second{};
    bool changed = false;
    arena.set_metadata_writer([&](std::uint32_t, std::optional<std::uint32_t>) {
        if (changed) return;
        changed = true;
        first.line_count = 0;
        first.points = {};
        templates.second.line_count = -1;
    });
    initialize(first, second, templates, arena);
    arena.set_metadata_writer({});
    require(first.point_count == 2 && word(arena, first.expanded_points, 0) == points[3],
        "first expander captured inputs before allocator mutated destination");
    require(second.line_count == -1 && second.point_count == -2 &&
        !second.expanded_points && !second.expanded_colors,
        "second template is copied only after first expansion returns");
    TaskWorkArena small;
    // Leave 64 bytes: point allocation needs 96, color allocation needs 64.
    const auto pressure = small.allocate(TaskWorkArena::usable_bytes - 96).work;
    require(bool(pressure), "actual arena pressure");
    model.line_count = 2;
    try {
        expand(model, small);
        require(false, "null point destination must fail");
    } catch (const Error &error) {
        require(error.boundary == Boundary::PointWrite && !model.expanded_points &&
            model.expanded_colors && model.point_count == 4 && !model.texture,
            "failed first allocation preserves second allocation and source output prefix");
    }
    TaskWorkArena invalid_arena;
    model.points = {&points, INT32_MAX};
    model.colors = {&colors, 0};
    const std::vector<std::int16_t> positive_indices{1, 1};
    model.indices = &positive_indices;
    model.line_count = 1;
    try {
        expand(model, invalid_arena);
        require(false, "invalid interior view must fail");
    } catch (const Error &error) {
        require(error.boundary == Boundary::PointRead && error.vertex == 0 && error.component == 0,
                "invalid view uses wide host bounds arithmetic and fails at point read");
    }
    TaskWorkArena prefix_arena;
    const std::vector<std::uint32_t> short_points{0x12345678};
    model.indices = &indices;
    model.points = {&short_points, 0};
    model.colors = {&colors, 0};
    model.line_count = 1;
    try {
        expand(model, prefix_arena);
        require(false, "missing second component must fail");
    } catch (const Error &error) {
        require(error.boundary == Boundary::PointRead && error.vertex == 0 && error.component == 1 &&
            word(prefix_arena, model.expanded_points, 0) == short_points[0],
            "source word-copy prefix remains when a later input component is unavailable");
    }
}
} // namespace
int main() {
    try { run(); }
    catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
    return 0;
}
