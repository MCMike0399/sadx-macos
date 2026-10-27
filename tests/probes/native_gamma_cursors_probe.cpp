#include "player/gamma_cursors.hpp"
#include <algorithm>
#include <cstring>

namespace {
using namespace sadx;
using namespace sadx::gamma_cursors;
constexpr unsigned kOutputWords = 512;
void snapshot(const WireModel &model, TaskWorkArena &arena, std::uint32_t *out,
              unsigned descriptor, unsigned payload,
              const std::vector<std::uint32_t> &points,
              const std::vector<std::uint32_t> &alternate_points,
              const std::vector<std::uint32_t> &colors,
              const std::vector<std::uint32_t> &alternate_colors,
              const std::vector<std::int16_t> &indices) {
    out[descriptor] = model.points.storage == &points ? 1 :
        model.points.storage == &alternate_points ? 2 : 0;
    out[descriptor + 1] = model.points.origin;
    out[descriptor + 2] = model.colors.storage == &colors ? 1 :
        model.colors.storage == &alternate_colors ? 2 : 0;
    out[descriptor + 3] = model.colors.origin;
    out[descriptor + 4] = model.indices == &indices ? 1 : 0;
    out[descriptor + 5] = static_cast<std::uint16_t>(model.line_count);
    out[descriptor + 6] = model.padding;
    out[descriptor + 7] = model.expanded_points ? model.expanded_points->data_offset() : 0;
    out[descriptor + 8] = model.expanded_colors ? model.expanded_colors->data_offset() : 0;
    out[descriptor + 9] = model.texture != nullptr;
    out[descriptor + 10] = model.point_count;
    if (model.expanded_points) {
        const auto bytes = std::min(384U, model.expanded_points->requested_size());
        (void)arena.read(model.expanded_points, 0, out + payload, bytes);
    }
    if (model.expanded_colors) {
        const auto bytes = std::min(128U, model.expanded_colors->requested_size());
        (void)arena.read(model.expanded_colors, 0, out + payload + 96, bytes);
    }
}
} // namespace
extern "C" int sadx_native_gamma_cursors(const std::uint32_t *in, std::uint32_t *out) {
    std::fill_n(out, kOutputWords, 0);
    TaskWorkArena arena;
    if (in[3]) {
        if (!arena.allocate(TaskWorkArena::usable_bytes - in[3] - TaskWorkArena::header_bytes))
            return 0;
    }
    const std::vector<std::uint32_t> points(in + 32, in + 80);
    const std::vector<std::uint32_t> colors(in + 80, in + 96);
    const std::vector<std::uint32_t> alternate_colors(in + 128, in + 144);
    const std::vector<std::uint32_t> alternate_points(in + 144, in + 192);
    std::vector<std::int16_t> indices;
    for (unsigned index = 96; index < 128; ++index)
        indices.push_back(static_cast<std::int16_t>(in[index]));
    Templates templates{};
    templates.first.points = {&points, static_cast<std::int32_t>(in[5] * 3)};
    templates.first.colors = {&colors, static_cast<std::int32_t>(in[5])};
    templates.first.indices = &indices;
    templates.first.line_count = static_cast<std::int16_t>(in[1]);
    templates.first.padding = static_cast<std::uint16_t>(in[7]);
    templates.second = templates.first;
    templates.second.colors = {&alternate_colors, static_cast<std::int32_t>(in[5])};
    templates.second.line_count = static_cast<std::int16_t>(in[2]);
    templates.second.padding = static_cast<std::uint16_t>(in[8]);
    WireModel first = templates.first, second{};
    bool mutated = false;
    auto previous_free = arena.free_head();
    unsigned successful_allocations = 0;
    arena.set_metadata_writer([&](std::uint32_t, std::optional<std::uint32_t>) {
        if (arena.free_head() == previous_free) return;
        previous_free = arena.free_head();
        ++successful_allocations;
        if (successful_allocations == 2)
            out[3] = first.expanded_points ? first.expanded_points->data_offset() : 0;
        if (successful_allocations == 4)
            out[4] = second.expanded_points ? second.expanded_points->data_offset() : 0;
        if (mutated) return;
        mutated = true;
        if (in[4] & 1) {
            first.points = {&alternate_points, 0};
            first.colors = {&alternate_colors, 0};
            first.indices = nullptr;
            first.line_count = 0;
        }
        if (in[4] & 2)
            templates.second.line_count = static_cast<std::int16_t>(in[6]);
        if (in[4] & 4) {
            templates.second.points = {&alternate_points, static_cast<std::int32_t>(in[5] * 3)};
            templates.second.colors = {&colors, static_cast<std::int32_t>(in[5])};
        }
    });
    try {
        if (in[0]) initialize(first, second, templates, arena);
        else expand(first, arena);
    } catch (const Error &error) {
        out[0] = static_cast<unsigned>(error.boundary) + 1;
        out[1] = error.vertex;
        out[2] = error.component;
    }
    arena.set_metadata_writer({});
    snapshot(first, arena, out, 16, 128, points, alternate_points,
             colors, alternate_colors, indices);
    snapshot(second, arena, out, 32, 256, points, alternate_points,
             colors, alternate_colors, indices);
    return 1;
}
