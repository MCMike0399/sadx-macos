#include "player/player_vertex_weld.hpp"
#include "vertex_weld_fixture.hpp"
#include <algorithm>

extern "C" int sadx_native_player_vertex_weld(const std::uint32_t *input, std::uint32_t *output) {
    using namespace sadx;
    std::fill_n(output, 192, 0);
    const auto pack = weld_fixture::pack(input);
    character_pack::SceneState scene(pack);
    weld_fixture::topology(scene, input);
    TaskWorkArena arena;
    if (input[9]) {
        const auto filler = arena.allocate(TaskWorkArena::usable_bytes - input[9] - 32);
        if (!filler)
            return 0;
    }
    TaskWorkHandle previous;
    if (input[8]) {
        previous = arena.allocate(input[8]).work;
        if (!previous)
            return 0;
        if (!arena.write(previous, 0, input + 80, input[8]))
            return 0;
    }
    std::vector<std::uint16_t> pairs;
    for (unsigned index = 12; index < 20; ++index)
        pairs.push_back(static_cast<std::uint16_t>(input[index]));
    PlayerJoinVertex join{};
    join.object = weld_fixture::object(scene, input[5]);
    join.source_object = weld_fixture::object(scene, input[3]);
    join.destination_object = weld_fixture::object(scene, input[4]);
    join.vertex_count = static_cast<std::uint8_t>(input[2]);
    join.input_mode = static_cast<std::int8_t>(input[1]);
    join.source_depth = static_cast<std::uint8_t>(input[6]);
    join.destination_depth = static_cast<std::uint8_t>(input[7]);
    join.original_vertices = previous;
    join.vertex_pairs = input[30] ? nullptr : &pairs;
    PlayerJoinTable table{{join, {}}};
    auto mode = static_cast<std::int16_t>(input[0]);
    try {
        if (mode == 0) {
            vertex_weld::initialize(table, arena, scene);
            mode = 1;
        } else if (mode == 1) {
            output[0] = 2; // Explicit unrecovered CalcMMMatrix boundary.
        } else {
            vertex_weld::restore(table, arena, scene);
        }
    } catch (const vertex_weld::Error &) {
        output[0] = 1;
    }
    const auto &result = table.rows[0];
    output[1] = static_cast<std::uint16_t>(mode);
    output[2] = result.original_vertices ? result.original_vertices->data_offset() : 0;
    output[3] = previous ? previous->data_offset() : 0;
    output[4] = previous && previous->released();
    output[5] = result.source_depth;
    output[6] = result.destination_depth;
    output[7] = static_cast<std::uint8_t>(result.input_mode);
    output[8] = result.vertex_count;
    output[9] = result.original_vertices ? result.original_vertices->requested_size() : 0;
    output[10] = arena.free_head() ? *arena.free_head() + 1 : 0;
    output[11] = previous ? previous->requested_size() : 0;
    for (unsigned vector = 0; vector < 4; ++vector) {
        const auto &values = scene.geometry.vector_words(10 + vector);
        std::copy(values.begin(), values.end(), output + 32 + vector * 12);
    }
    if (result.original_vertices) {
        const auto count = std::min(192U, result.original_vertices->requested_size());
        std::copy_n(arena.source_bytes().data() + result.original_vertices->data_offset(), count,
                    reinterpret_cast<std::uint8_t *>(output + 80));
    }
    if (previous)
        std::copy_n(arena.source_bytes().data() + previous->data_offset(), input[8],
                    reinterpret_cast<std::uint8_t *>(output + 128));
    output[16] = table.rows[0].object == join.object;
    output[17] = table.rows[0].source_object == join.source_object;
    output[18] = table.rows[0].destination_object == join.destination_object;
    output[19] = table.rows[0].vertex_pairs == join.vertex_pairs;
    return 1;
}

extern "C" int sadx_native_player_node_index(const std::uint32_t *input, std::int32_t *output) {
    const auto pack = weld_fixture::pack(input);
    sadx::character_pack::SceneState scene(pack);
    weld_fixture::topology(scene, input);
    try {
        output[0] =
            sadx::vertex_weld::node_index(weld_fixture::object(scene, input[5]),
                                          weld_fixture::object(scene, input[3]), scene.objects);
        output[1] = 0;
    } catch (const sadx::vertex_weld::Error &) {
        output[0] = 0;
        output[1] = 1;
    }
    return 1;
}
