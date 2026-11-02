#include "player/player_vertex_weld.hpp"
#include <array>
#include <cassert>
#include <iostream>
#include <set>

namespace {
void check_dispatch() {
    for (int raw = -32768; raw <= 32767; ++raw) {
        auto mode = static_cast<std::int16_t>(raw);
        unsigned active = 0;
        unsigned initialized = 0;
        unsigned restored = 0;
        sadx::vertex_weld::update_prefix(
            mode, [&] { ++active; },
            [&] {
                assert(mode == 0);
                ++initialized;
            },
            [&] { ++restored; });
        assert(active == (raw == 1));
        assert(initialized == (raw == 0));
        assert(restored == (raw != 0 && raw != 1));
        assert(mode == (raw == 0 ? 1 : raw));
    }
    for (const std::int16_t initial : {0, 1, -1}) {
        auto mode = initial;
        unsigned reached = 0;
        try {
            sadx::vertex_weld::update_prefix(
                mode, [&] { throw 1; }, [&] { throw 2; }, [&] { throw 3; });
        } catch (int boundary) {
            reached = boundary;
        }
        assert(reached == (initial == 1 ? 1U : initial == 0 ? 2U : 3U));
        assert(mode == initial);
    }
}
} // namespace

int main(int argc, char **argv) {
    assert(argc == 2);
    check_dispatch();
    using namespace sadx;
    using namespace sadx::character_pack;
    const auto pack = CharacterPack::load(argv[1]);
    SceneState scene(pack);
    TaskWorkArena arena;
    std::array<ObjectResource *, 2> objects{};
    std::array<ModelResource, 2> models{};
    unsigned found = 0;
    std::set<ResourceId> used;
    for (ResourceId id = 1; id <= pack.record_count() && found < 2; ++id) {
        if (pack.record(id).kind != ResourceKind::Object)
            continue;
        auto &object = scene.objects.object(id);
        if (!object.model)
            continue;
        const auto model = pack.model(object.model);
        if (model.vertex_count < 2 || !model.normals || model.points == model.normals ||
            used.count(model.points) || used.count(model.normals))
            continue;
        objects[found] = &object;
        models[found++] = model;
        used.insert(model.points);
        used.insert(model.normals);
    }
    assert(found == 2);
    const std::array<ResourceId, 4> vectors{models[0].points, models[0].normals, models[1].points,
                                            models[1].normals};
    std::array<std::vector<std::uint32_t>, 4> expected;
    std::array<const std::uint32_t *, 4> storage{};
    for (unsigned vector = 0; vector < vectors.size(); ++vector) {
        auto &words = scene.geometry.vector_words(vectors[vector]);
        for (unsigned word = 0; word < words.size(); ++word)
            words[word] = 0x31000000U + vector * 0x10000U + word * 97U;
        expected[vector] = words;
        storage[vector] = words.data();
    }
    const std::vector<std::uint16_t> pairs{1, 0, 0, 1};
    PlayerJoinVertex join{};
    join.object = objects[0];
    join.source_object = objects[0];
    join.destination_object = objects[1];
    join.vertex_count = 2;
    join.input_mode = 1;
    join.vertex_pairs = &pairs;
    PlayerJoinTable table{{join, {}}};
    vertex_weld::initialize(table, arena, scene);
    const auto backup = table.rows[0].original_vertices;
    assert(backup && backup->requested_size() == 96 && !backup->released());
    for (unsigned vertex = 0; vertex < 2; ++vertex) {
        for (unsigned vector = 0; vector < 4; ++vector) {
            const auto point = pairs[vertex * 2 + vector / 2];
            for (unsigned axis = 0; axis < 3; ++axis) {
                const auto offset = vertex * 48 + vector * 12 + axis * 4;
                std::uint32_t word;
                assert(arena.read(backup, offset, &word, 4));
                assert(word == expected[vector][point * 3 + axis]);
                // Teardown must read the current arena block, not an initializer snapshot.
                word = 0x72000000U + vertex * 0x10000U + vector * 0x100U + axis;
                assert(arena.write(backup, offset, &word, 4));
                expected[vector][point * 3 + axis] = word;
            }
        }
    }
    for (const auto resource : vectors) {
        auto &words = scene.geometry.vector_words(resource);
        for (unsigned word = 0; word < 6; ++word)
            words[word] = 0x11000000U + word;
    }
    vertex_weld::restore(table, arena, scene);
    assert(backup->released() && !table.rows[0].original_vertices);
    for (unsigned vector = 0; vector < vectors.size(); ++vector) {
        const auto &actual = scene.geometry.vector_words(vectors[vector]);
        assert(actual.data() == storage[vector]);
        assert(actual == expected[vector]);
    }
    // Repeated cleanup has no backup and must not reinterpret the pointer as live.
    vertex_weld::restore(table, arena, scene);
    // A fresh initialization observes current geometry and owns a new handle,
    // even if the arena can reuse an earlier source address.
    vertex_weld::initialize(table, arena, scene);
    const auto second = table.rows[0].original_vertices;
    assert(second && second != backup && !second->released());
    vertex_weld::initialize(table, arena, scene);
    const auto third = table.rows[0].original_vertices;
    assert(third && third != second && !second->released());
    vertex_weld::restore(table, arena, scene);
    assert(third->released() && !second->released());
    vertex_weld::initialize(table, arena, scene);
    const auto fourth = table.rows[0].original_vertices;
    const auto replacement = arena.allocate(96).work;
    assert(fourth && replacement);
    bool reentered = false;
    arena.set_metadata_writer([&](std::uint32_t, std::optional<std::uint32_t>) {
        if (!reentered) {
            reentered = true;
            table.rows[0].original_vertices = replacement;
        }
    });
    vertex_weld::restore(table, arena, scene);
    arena.set_metadata_writer({});
    assert(reentered && fourth->released() && !replacement->released());
    assert(!table.rows[0].original_vertices);
    std::cout << "Weld backups and restoration borrow all four actual scene arrays\n";
}
