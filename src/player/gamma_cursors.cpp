#include "player/gamma_cursors.hpp"

namespace sadx::gamma_cursors {
namespace {
// NOTSA: checked typed imported-memory read; absence is a boundary failure.
std::uint32_t read_word(WordView view, std::int32_t offset,
                        Boundary boundary, std::int32_t vertex, unsigned component) {
    const auto position = static_cast<std::int64_t>(view.origin) + offset;
    const auto *words = view.storage;
    if (!words || position < 0 || static_cast<std::uint64_t>(position) >= words->size())
        throw Error(boundary, vertex, component);
    return (*words)[static_cast<std::size_t>(position)];
}
// NOTSA: publish each source word separately so a failed later access retains the prefix.
void write_word(TaskWorkArena &arena, const TaskWorkHandle &allocation, std::uint32_t offset,
                std::uint32_t word, Boundary boundary, std::int32_t vertex, unsigned component) {
    const auto result = arena.write(allocation, offset, &word, sizeof(word));
    if (!result)
        throw Error(boundary, vertex, component, result.failure);
}
} // namespace

void expand(WireModel &model, TaskWorkArena &arena) {
    const auto lines = static_cast<std::int32_t>(model.line_count);
    const auto points = model.points;
    const auto colors = model.colors;
    const auto *indices = model.indices;
    const auto count = lines * 2;
    constexpr std::uint32_t kPointBytes = 12, kColorBytes = 4;
    // Preserve both allocator calls and publications even when allocation fails.
    // Original quirk: overwritten destination buffers are not released here.
    const auto point_buffer = arena.allocate(static_cast<std::uint32_t>(count) * kPointBytes).work;
    model.expanded_points = point_buffer;
    const auto color_buffer = arena.allocate(static_cast<std::uint32_t>(count) * kColorBytes).work;
    model.expanded_colors = color_buffer;
    model.texture = nullptr;
    model.point_count = count;
    for (std::int32_t vertex = 0; vertex < count; ++vertex) {
        if (!indices || static_cast<std::size_t>(vertex) >= indices->size())
            throw Error(Boundary::IndexRead, vertex, 0);
        const auto index = static_cast<std::int32_t>((*indices)[static_cast<std::size_t>(vertex)]);
        for (unsigned component = 0; component < 3; ++component) {
            const auto word = read_word(points, index * 3 + static_cast<std::int32_t>(component),
                                        Boundary::PointRead, vertex, component);
            write_word(arena, point_buffer, static_cast<std::uint32_t>(vertex) * kPointBytes +
                       component * kColorBytes, word, Boundary::PointWrite, vertex, component);
        }
        const auto color = read_word(colors, index, Boundary::ColorRead, vertex, 0);
        write_word(arena, color_buffer, static_cast<std::uint32_t>(vertex) * kColorBytes,
                   color, Boundary::ColorWrite, vertex, 0);
    }
}

void initialize(WireModel &first, WireModel &second, const Templates &templates,
                TaskWorkArena &arena) {
    first = templates.first;
    expand(first, arena);
    second = templates.second;
    expand(second, arena);
}
} // namespace sadx::gamma_cursors
