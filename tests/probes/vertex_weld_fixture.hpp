#pragma once
#include "engine/character_geometry.hpp"

namespace weld_fixture {
using namespace sadx::character_pack;
using Bytes = std::vector<std::uint8_t>;
inline void append(Bytes &bytes, std::uint32_t value) {
    for (unsigned byte = 0; byte < 4; ++byte)
        bytes.push_back(static_cast<std::uint8_t>(value >> (byte * 8)));
}
inline Bytes words(std::initializer_list<std::uint32_t> values) {
    Bytes bytes;
    for (const auto value : values)
        append(bytes, value);
    return bytes;
}
inline void record(Bytes &bytes, ResourceKind kind, const Bytes &payload) {
    append(bytes, static_cast<std::uint32_t>(kind));
    append(bytes, static_cast<std::uint32_t>(payload.size()));
    bytes.insert(bytes.end(), payload.begin(), payload.end());
}
inline CharacterPack pack(const std::uint32_t *input) {
    Bytes bytes{'S', 'D', 'X', 'A', 'C', 'T', '0', '1'};
    for (const auto value : {13, 2, 100, 1, 101, 4})
        append(bytes, value);
    record(bytes, ResourceKind::Action, words({2, 3}));
    record(bytes, ResourceKind::Object, words({0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0}));
    record(bytes, ResourceKind::Motion, words({5, 1, 3 + (2 << 16), 3}));
    record(bytes, ResourceKind::Action, words({2, 3}));
    auto tracks = words({3});
    tracks.resize(52);
    record(bytes, ResourceKind::Tracks, tracks);
    record(bytes, ResourceKind::Model,
           words({10, input[31] & 1 ? 0U : 11U, 4, 0, 0, 0, 0, 0, 0, 0, 0}));
    record(bytes, ResourceKind::Object, words({0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8}));
    record(bytes, ResourceKind::Object, words({0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}));
    record(bytes, ResourceKind::Model,
           words({12, input[31] & 2 ? 0U : 13U, 4, 0, 0, 0, 0, 0, 0, 0, 0}));
    for (unsigned vector = 0; vector < 4; ++vector) {
        auto payload = words({4});
        for (unsigned component = 0; component < 12; ++component)
            append(payload, input[32 + vector * 12 + component]);
        record(bytes, ResourceKind::Vectors, payload);
    }
    return CharacterPack::decode(bytes);
}
inline ResourceId object_id(unsigned index) {
    constexpr ResourceId ids[]{2, 7, 8, 0};
    return ids[index];
}
inline void topology(SceneState &scene, const std::uint32_t *input) {
    for (unsigned index = 0; index < 3; ++index) {
        auto &object = scene.objects.object(object_id(index));
        object.child = object_id(input[20 + index * 2]);
        object.sibling = object_id(input[21 + index * 2]);
        object.evaluation_flags = input[26 + index];
        if (input[29] & (1U << index))
            object.model = 0;
    }
}
inline ObjectResource *object(SceneState &scene, unsigned index) {
    const auto id = object_id(index);
    return id ? &scene.objects.object(id) : nullptr;
}
} // namespace weld_fixture
