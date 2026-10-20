#include "engine/character_pack.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {
using namespace sadx::character_pack;
using Bytes = std::vector<std::uint8_t>;
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
void put(Bytes& bytes, std::size_t offset, std::uint32_t value) {
    for (std::size_t byte = 0; byte < 4; ++byte) {
        bytes.at(offset + byte) = static_cast<std::uint8_t>(value >> (byte * 8));
    }
}
std::uint32_t get(const Bytes& bytes, std::size_t offset) {
    std::uint32_t result = 0;
    for (std::size_t byte = 0; byte < 4; ++byte) {
        result |= std::uint32_t(bytes.at(offset + byte)) << (byte * 8);
    }
    return result;
}
void append(Bytes& bytes, std::uint32_t value) {
    const auto end = bytes.size();
    bytes.resize(end + 4);
    put(bytes, end, value);
}
void add_record(Bytes& bytes, ResourceKind kind, const Bytes& payload) {
    append(bytes, static_cast<std::uint32_t>(kind));
    append(bytes, static_cast<std::uint32_t>(payload.size()));
    bytes.insert(bytes.end(), payload.begin(), payload.end());
}
Bytes minimal_pack() {
    Bytes bytes{'S', 'D', 'X', 'A', 'C', 'T', '0', '1'};
    for (auto value : {5, 2, 100, 1, 101, 4}) {
        append(bytes, value);
    }
    Bytes action(8);
    put(action, 0, 2);
    put(action, 4, 3);
    add_record(bytes, ResourceKind::Action, action);
    add_record(bytes, ResourceKind::Object, Bytes(52));
    Bytes motion(16);
    put(motion, 0, 5);
    put(motion, 4, 1);
    put(motion, 8, 3 + (2 << 16));
    put(motion, 12, 1);
    add_record(bytes, ResourceKind::Motion, motion);
    add_record(bytes, ResourceKind::Action, action);
    Bytes tracks(20);
    put(tracks, 0, 1);
    add_record(bytes, ResourceKind::Tracks, tracks);
    return bytes;
}
std::size_t record_offset(const Bytes& bytes, std::uint32_t id) {
    std::size_t cursor = 16 + get(bytes, 12) * 8;
    for (std::uint32_t current = 1; current < id; ++current) {
        cursor += 8 + get(bytes, cursor + 4);
    }
    return cursor;
}
std::size_t first_kind(const Bytes& bytes, ResourceKind kind) {
    for (std::uint32_t id = 1; id <= get(bytes, 8); ++id) {
        const auto offset = record_offset(bytes, id);
        if (get(bytes, offset) == static_cast<std::uint32_t>(kind)) {
            return offset;
        }
    }
    throw std::runtime_error("fixture is missing resource kind");
}
std::size_t rejected = 0;
void reject(const Bytes& bytes, const char* reason) {
    try {
        CharacterPack::decode(bytes);
    } catch (const std::runtime_error& error) {
        require(std::string(error.what()).find("character action pack:") == 0,
                "malformed file must report the asset boundary");
        if (std::string(error.what()).find(reason) == std::string::npos) {
            throw std::runtime_error(std::string("expected ") + reason + "; got " + error.what());
        }
        ++rejected;
        return;
    }
    throw std::runtime_error("malformed character pack was accepted");
}
void mutate(const Bytes& original, std::size_t offset, std::uint32_t value, const char* reason) {
    auto bytes = original;
    put(bytes, offset, value);
    reject(bytes, reason);
}
void synthetic_checks() {
    const auto bytes = minimal_pack();
    const auto pack = CharacterPack::decode(bytes);
    const auto clear_pose = pack.action(StartupAction::ClearPose);
    require(pack.record_count() == 5 && clear_pose.identity == 1, "minimal typed pack loads");
    require(pack.resolve(clear_pose).object == 2 && pack.motion(3).part_count == 1,
            "handles resolve to typed object/motion graph");
    require(pack.object(2).child == 0 && pack.object(2).position[0] == 0,
            "absent resources and object fields survive decoding");
    for (std::size_t size = 0; size < bytes.size(); ++size) {
        reject(Bytes(bytes.begin(), bytes.begin() + size), "");
    }
    mutate(bytes, 0, 0, "header");
    mutate(bytes, 8, 0, "count");
    mutate(bytes, 8, 4000000000U, "count");
    mutate(bytes, 12, 1, "count");
    mutate(bytes, 24, 100, "duplicate");
    mutate(bytes, 16, 102, "descriptor");
    mutate(bytes, 20, 0, "out of range");
    mutate(bytes, 20, 6, "out of range");
    mutate(bytes, 20, 2, "type");
    mutate(bytes, record_offset(bytes, 1), 15, "kind");
    mutate(bytes, record_offset(bytes, 1) + 8, 3, "type");
    mutate(bytes, record_offset(bytes, 1) + 8, 6, "out of range");
    mutate(bytes, record_offset(bytes, 1) + 8, 0, "required");
    mutate(bytes, record_offset(bytes, 2) + 8 + 44, 2, "cyclic");
    mutate(bytes, record_offset(bytes, 2) + 8 + 48, 2, "cyclic");
    mutate(bytes, record_offset(bytes, 2) + 8, 64, "part count");
    mutate(bytes, record_offset(bytes, 3) + 8 + 8, 4, "MDATA2");
    mutate(bytes, record_offset(bytes, 3) + 8 + 12, 2, "array count");
    mutate(bytes, record_offset(bytes, 5) + 8, 2, "array count");
    mutate(bytes, record_offset(bytes, 5) + 8 + 12, 1, "required");
    auto extra = bytes;
    extra.push_back(0);
    reject(extra, "trailing");
    try {
        pack.resolve(sadx::ActionHandle{std::uint64_t(1) << 40});
        throw std::logic_error("oversized action handle was accepted");
    } catch (const std::runtime_error&) {
        ++rejected;
    }
}
void topology_checks() {
    for (std::uint32_t topology = 0; topology < 4; ++topology) {
        auto bytes = minimal_pack();
        put(bytes, 8, 10);
        put(bytes, record_offset(bytes, 2) + 8 + 4, 6);
        Bytes model(44);
        put(model, 0, 7);
        put(model, 8, 4);
        put(model, 12, 8);
        put(model, 16, 9);
        put(model, 20, 1 + (1 << 16));
        add_record(bytes, ResourceKind::Model, model);
        Bytes points(4 + 4 * 12);
        put(points, 0, 4);
        add_record(bytes, ResourceKind::Vectors, points);
        Bytes meshes(4 + 28);
        put(meshes, 0, 1);
        put(meshes, 4, (topology << 14) + (1 << 16));
        put(meshes, 8, 10);
        add_record(bytes, ResourceKind::Meshsets, meshes);
        Bytes materials(4 + 20);
        put(materials, 0, 1);
        add_record(bytes, ResourceKind::Materials, materials);
        const auto corners = topology == 1 ? 4U : 3U;
        const auto headers = topology >= 2 ? 1U : 0U;
        Bytes indices(4 + (corners + headers) * 2);
        put(indices, 0, corners + headers);
        std::size_t cursor = 4;
        if (headers) {
            const auto header = topology == 3 ? 32771U : 3U;
            indices[cursor++] = static_cast<std::uint8_t>(header);
            indices[cursor++] = static_cast<std::uint8_t>(header >> 8);
        }
        for (std::uint32_t corner = 0; corner < corners; ++corner) {
            indices[cursor++] = static_cast<std::uint8_t>(corner);
            indices[cursor++] = 0;
        }
        add_record(bytes, ResourceKind::Indices, indices);
        const auto pack = CharacterPack::decode(bytes);
        require(pack.model(6).vertex_count == 4, "all four Ninja mesh topologies decode");
        auto invalid = bytes;
        const auto first = record_offset(bytes, 10) + 8 + 4;
        invalid.at(first + headers * 2) = 4;
        reject(invalid, "vertex index");
        if (headers) {
            invalid = bytes;
            invalid.at(first) = 2;
            reject(invalid, "corner count");
        }
    }
}
void real_pack_checks(const char* path) {
    std::ifstream input(path, std::ios::binary);
    require(bool(input), "actual imported startup action pack must exist");
    const Bytes bytes{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    const auto pack = CharacterPack::load(path);
    require(bytes.size() == 90756 && pack.record_count() == 358, "complete real imported resource graph");
    const auto clear = pack.resolve(pack.action(StartupAction::ClearPose));
    const auto transition = pack.resolve(pack.action(StartupAction::ClearPoseTransition));
    require(clear.object == transition.object && clear.motion != transition.motion,
            "clear-pose actions share the actual model graph and retain distinct motions");
    require(pack.motion(clear.motion).frame_count == 65 &&
                pack.motion(transition.motion).frame_count == 11 &&
                pack.motion(clear.motion).part_count == 73,
            "actual source motion frame and part counts are retained");
    std::size_t retained_bytes = 0;
    std::size_t cursor = 32;
    std::size_t models = 0;
    for (std::uint32_t id = 1; id <= pack.record_count(); ++id) {
        const auto& record = pack.record(id);
        const auto length = get(bytes, cursor + 4);
        require(record.payload.size() == length &&
                    std::equal(record.payload.begin(), record.payload.end(), bytes.begin() + cursor + 8),
                "every resource retains its exact typed pack payload");
        retained_bytes += length;
        cursor += 8 + length;
        if (record.kind == ResourceKind::Model) {
            const auto model = pack.model(id);
            require(model.points && model.vertex_count && model.meshsets && model.materials,
                    "real model resources retain mesh/material/vector references");
            ++models;
        }
    }
    require(retained_bytes + 32 + 358 * 8 == bytes.size() && models > 1,
            "entire renderable graph is owned without executable padding");
    const auto object = first_kind(bytes, ResourceKind::Object) + 8;
    mutate(bytes, object + 44, 2, "cyclic");
    const auto model = first_kind(bytes, ResourceKind::Model) + 8;
    mutate(bytes, model, 0, "required");
    mutate(bytes, model + 8, 4000000000U, "negative vertex");
    mutate(bytes, model + 8, get(bytes, model + 8) + 1, "array count");
    mutate(bytes, model + 40, 1, "runtime buffer");
    const auto mesh = first_kind(bytes, ResourceKind::Meshsets) + 8 + 4;
    mutate(bytes, mesh + 4, 1, "type");
    mutate(bytes, mesh, get(bytes, mesh) | 16383, "material index");
    mutate(bytes, mesh + 24, 1, "runtime buffer");
    const auto indices = first_kind(bytes, ResourceKind::Indices) + 8;
    auto invalid_vertex = bytes;
    invalid_vertex.at(indices + 6) = 255;
    invalid_vertex.at(indices + 7) = 255;
    reject(invalid_vertex, "vertex index");
    mutate(bytes, indices, get(bytes, indices) + 1, "array length");
    const auto tracks = first_kind(bytes, ResourceKind::Tracks) + 8;
    mutate(bytes, tracks + 12, get(bytes, tracks + 12) + 1, "array count");
    mutate(bytes, first_kind(bytes, ResourceKind::Motion) + 8 + 12, 72, "array count");
    auto copied = pack;
    require(copied.resolve(copied.action(StartupAction::ClearPose)).object == clear.object,
            "owned resource IDs remain valid after copying the pack");
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc == 2, "usage: character-pack-test IMPORTED_ACTION_PACK");
        synthetic_checks();
        topology_checks();
        real_pack_checks(argv[1]);
        std::cout << "character pack: actual resource graph, typed views and " << rejected
                  << " malformed/invalid cases pass\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
