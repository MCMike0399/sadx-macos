#include "engine/character_pack.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

namespace sadx::character_pack {
namespace {
// NOTSA: schema descriptors use source Characters IDs and source action-table indices.
constexpr std::array<std::array<std::uint32_t, 2>, 7> kTitleActionDescriptors{{
    {0, 100}, {0, 101}, {3, 40}, {3, 41}, {7, 4}, {7, 29}, {7, 30}}};
// NOTSA: domain0 names a DLL action index; domain1 names a constructor-derived selection action.
constexpr std::array<std::array<std::uint32_t, 3>, 19> kSelectionActionDescriptors{{
    {0,0,5}, {0,0,100}, {1,0,0},
    {0,2,17}, {0,2,39}, {1,2,0},
    {0,3,81}, {0,3,40}, {1,3,0},
    {0,5,70}, {0,5,29}, {1,5,0},
    {0,7,16}, {0,7,29}, {0,7,30}, {1,7,0},
    {0,6,2}, {1,6,0}, {1,6,1}}};
// NOTSA: decode the documented little-endian pack schema without host-layout casts.
std::uint16_t word(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(bytes.at(offset) | (bytes.at(offset + 1) << 8));
}
std::uint32_t integer(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(word(bytes, offset)) |
           (static_cast<std::uint32_t>(word(bytes, offset + 2)) << 16);
}
float scalar(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    const auto bits = integer(bytes, offset);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
[[noreturn]] void invalid(const std::string& reason) {
    throw std::runtime_error("character action pack: " + reason);
}
const std::vector<std::uint8_t>& payload(const CharacterPack& pack, ResourceId id,
                                       ResourceKind kind, std::size_t size) {
    const auto& record = pack.record(id);
    if (record.kind != kind || record.payload.size() != size) {
        invalid("record " + std::to_string(id) + " has wrong type or length");
    }
    return record.payload;
}
std::uint32_t array_stride(ResourceKind kind) {
    switch (kind) {
    case ResourceKind::Materials:
        return 20;
    case ResourceKind::Vectors:
        return 12;
    case ResourceKind::Colors:
        return 4;
    case ResourceKind::Uv:
        return 4;
    case ResourceKind::Indices:
        return 2;
    case ResourceKind::Attributes:
        return 4;
    case ResourceKind::PositionKeys:
        return 16;
    case ResourceKind::RotationKeys:
        return 16;
    case ResourceKind::Meshsets:
        return 28;
    case ResourceKind::Tracks:
        return 16;
    default:
        return 0;
    }
}

// NOTSA: format validation is separate from game behaviour and completes before handles escape.
class Validator {
  public:
    explicit Validator(const CharacterPack& pack)
        : pack_(pack), edges_(pack.record_count() + 1) {}
    void records() {
        for (ResourceId id = 1; id <= pack_.record_count(); ++id) {
            const auto& resource = pack_.record(id);
            const auto& bytes = resource.payload;
            const auto stride = array_stride(resource.kind);
            if (stride) {
                if (bytes.size() < 4 || bytes.size() != 4 + std::uint64_t(integer(bytes, 0)) * stride) {
                    invalid("record " + std::to_string(id) + " has invalid array length");
                }
            }
            switch (resource.kind) {
            case ResourceKind::Action:
                validate_action(id);
                break;
            case ResourceKind::Object:
                validate_object(id);
                break;
            case ResourceKind::Model:
                validate_model(id);
                break;
            case ResourceKind::Motion:
                validate_motion(id);
                break;
            case ResourceKind::Meshsets:
                validate_meshsets(id);
                break;
            case ResourceKind::Tracks:
                validate_tracks(id);
                break;
            default:
                break;
            }
        }
    }
    void graphs() const {
        std::vector<std::uint8_t> state(edges_.size(), 0);
        std::vector<std::pair<ResourceId, std::size_t>> stack;
        for (ResourceId root = 1; root < edges_.size(); ++root) {
            if (state[root]) {
                continue;
            }
            state[root] = 1;
            stack.emplace_back(root, 0);
            while (!stack.empty()) {
                auto& cursor = stack.back();
                if (cursor.second == edges_[cursor.first].size()) {
                    state[cursor.first] = 2;
                    stack.pop_back();
                    continue;
                }
                const auto child = edges_[cursor.first][cursor.second++];
                if (state[child] == 1) {
                    invalid("cyclic resource traversal at record " + std::to_string(child));
                }
                if (!state[child]) {
                    state[child] = 1;
                    stack.emplace_back(child, 0);
                }
            }
        }
    }
    void animation_parts() const {
        // A shared subtree is counted each time Ninja visits it, rather than once per record.
        std::vector<std::uint64_t> counts(edges_.size(), 0);
        std::vector<bool> counted(edges_.size(), false);
        for (ResourceId id = 1; id < edges_.size(); ++id) {
            if (pack_.record(id).kind != ResourceKind::Object || counted[id]) {
                continue;
            }
            std::vector<std::pair<ResourceId, bool>> stack{{id, false}};
            while (!stack.empty()) {
                const auto [current, expanded] = stack.back();
                stack.pop_back();
                if (counted[current]) {
                    continue;
                }
                const auto object = pack_.object(current);
                if (!expanded) {
                    stack.emplace_back(current, true);
                    for (auto child : {object.child, object.sibling}) {
                        if (child && !counted[child]) {
                            stack.emplace_back(child, false);
                        }
                    }
                    continue;
                }
                counts[current] = ((object.evaluation_flags & 64) == 0 ? 1 : 0) +
                                  counts[object.child] + counts[object.sibling];
                if (counts[current] > std::numeric_limits<std::uint32_t>::max()) {
                    invalid("object animation traversal exceeds supported part count");
                }
                counted[current] = true;
            }
        }
        for (ResourceId id = 1; id < edges_.size(); ++id) {
            if (pack_.record(id).kind == ResourceKind::Action) {
                const auto action = pack_.resolve(ActionHandle{id});
                if (counts[action.object] != pack_.motion(action.motion).part_count) {
                    invalid("action " + std::to_string(id) + " animation part count differs from object graph");
                }
            }
        }
    }

  private:
    void reference(ResourceId owner, ResourceId target, ResourceKind kind, bool required = false) {
        if (!target) {
            if (required) {
                invalid("record " + std::to_string(owner) + " is missing a required resource");
            }
            return;
        }
        if (pack_.record(target).kind != kind) {
            invalid("record " + std::to_string(owner) + " reference has wrong resource type");
        }
        edges_[owner].push_back(target);
    }
    void array_reference(ResourceId owner, ResourceId target, ResourceKind kind,
                         std::uint32_t count, bool required = false) {
        reference(owner, target, kind, required && count != 0);
        if (target) {
            const auto& bytes = pack_.record(target).payload;
            if (bytes.size() < 4 || integer(bytes, 0) != count ||
                bytes.size() != 4 + std::uint64_t(count) * array_stride(kind)) {
                invalid("record " + std::to_string(owner) + " referenced array count differs");
            }
        }
    }
    void validate_action(ResourceId id) {
        const auto& bytes = payload(pack_, id, ResourceKind::Action, 8);
        reference(id, integer(bytes, 0), ResourceKind::Object, true);
        reference(id, integer(bytes, 4), ResourceKind::Motion, true);
    }
    void validate_object(ResourceId id) {
        const auto value = pack_.object(id);
        reference(id, value.model, ResourceKind::Model);
        reference(id, value.child, ResourceKind::Object);
        reference(id, value.sibling, ResourceKind::Object);
    }
    void validate_model(ResourceId id) {
        const auto value = pack_.model(id);
        if (value.vertex_count > std::numeric_limits<std::int32_t>::max()) {
            invalid("model has a negative vertex count");
        }
        array_reference(id, value.points, ResourceKind::Vectors, value.vertex_count, true);
        array_reference(id, value.normals, ResourceKind::Vectors, value.vertex_count);
        array_reference(id, value.meshsets, ResourceKind::Meshsets, value.mesh_count, true);
        array_reference(id, value.materials, ResourceKind::Materials, value.material_count, true);
        if (integer(pack_.record(id).payload, 40)) {
            invalid("model contains an unsupported runtime buffer");
        }
        if (value.meshsets) {
            const auto& meshes = pack_.record(value.meshsets).payload;
            for (std::uint32_t index = 0; index < value.mesh_count; ++index) {
                const auto offset = 4 + index * 28;
                if ((word(meshes, offset) & 16383) >= value.material_count) {
                    invalid("mesh material index is outside model");
                }
                mesh_corners(id, meshes, offset, value.vertex_count);
            }
        }
    }
    std::uint32_t mesh_corners(ResourceId owner, const std::vector<std::uint8_t>& meshes,
                               std::size_t offset, std::uint32_t vertices) {
        const auto topology = word(meshes, offset) >> 14;
        const auto polygons = word(meshes, offset + 2);
        const auto indices = integer(meshes, offset + 4);
        reference(owner, indices, ResourceKind::Indices, polygons != 0);
        if (!indices) {
            return 0;
        }
        const auto& values = pack_.record(indices).payload;
        if (values.size() < 4) {
            invalid("mesh indices are truncated");
        }
        std::size_t cursor = 4;
        std::uint64_t corners = 0;
        for (std::uint32_t polygon = 0; polygon < polygons; ++polygon) {
            std::uint32_t size = topology == 0 ? 3 : 4;
            if (topology >= 2) {
                if (values.size() - cursor < 2) {
                    invalid("mesh primitive header is truncated");
                }
                size = word(values, cursor);
                if (topology == 3) {
                    size &= 32767;
                }
                cursor += 2;
            }
            if (size < 3 || std::uint64_t(size) * 2 > values.size() - cursor) {
                invalid("mesh primitive corner count is invalid");
            }
            for (std::uint32_t corner = 0; corner < size; ++corner) {
                if (word(values, cursor) >= vertices) {
                    invalid("mesh vertex index is outside model");
                }
                cursor += 2;
            }
            corners += size;
        }
        if (cursor != values.size() || corners > std::numeric_limits<std::uint32_t>::max()) {
            invalid("mesh index array length differs from primitives");
        }
        return static_cast<std::uint32_t>(corners);
    }
    void validate_meshsets(ResourceId id) {
        const auto& bytes = pack_.record(id).payload;
        const auto count = integer(bytes, 0);
        for (std::uint32_t index = 0; index < count; ++index) {
            const auto offset = 4 + index * 28;
            const auto polygons = word(bytes, offset + 2);
            const auto corners = mesh_corners(id, bytes, offset,
                                               std::numeric_limits<std::uint32_t>::max());
            array_reference(id, integer(bytes, offset + 8), ResourceKind::Attributes, polygons);
            array_reference(id, integer(bytes, offset + 12), ResourceKind::Vectors, polygons);
            array_reference(id, integer(bytes, offset + 16), ResourceKind::Colors, corners);
            array_reference(id, integer(bytes, offset + 20), ResourceKind::Uv, corners);
            if (integer(bytes, offset + 24)) {
                invalid("mesh contains an unsupported runtime buffer");
            }
        }
    }
    void validate_motion(ResourceId id) {
        const auto value = pack_.motion(id);
        if (value.channels != 3 || (value.interpolation & 15) != 2) {
            invalid("motion is not position/rotation MDATA2");
        }
        array_reference(id, value.tracks, ResourceKind::Tracks, value.part_count, true);
    }
    void validate_tracks(ResourceId id) {
        const auto& bytes = pack_.record(id).payload;
        const auto count = integer(bytes, 0);
        for (std::uint32_t index = 0; index < count; ++index) {
            const auto offset = 4 + index * 16;
            array_reference(id, integer(bytes, offset), ResourceKind::PositionKeys,
                            integer(bytes, offset + 8), true);
            array_reference(id, integer(bytes, offset + 4), ResourceKind::RotationKeys,
                            integer(bytes, offset + 12), true);
        }
    }
    const CharacterPack& pack_;
    std::vector<std::vector<ResourceId>> edges_;
};
} // namespace

CharacterPack CharacterPack::load(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        invalid("cannot open " + path.string());
    }
    const auto length = input.tellg();
    // NOTSA: bound resource allocations at this file boundary, before allocating or decoding.
    if (length < 0 || length > 256 * 1024 * 1024) {
        invalid("unsupported file size for " + path.string());
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(bytes.data()), length)) {
        invalid("cannot read " + path.string());
    }
    return decode(bytes);
}

CharacterPack CharacterPack::decode(const std::vector<std::uint8_t>& bytes) {
    const bool title = bytes.size() >= 16 && std::memcmp(bytes.data(), "SDXACT02", 8) == 0;
    const bool selection = bytes.size() >= 16 && std::memcmp(bytes.data(), "SDXACT03", 8) == 0;
    if (bytes.size() < 16 || (!selection && !title && std::memcmp(bytes.data(), "SDXACT01", 8) != 0)) {
        invalid("invalid SDXACT01 header");
    }
    const auto records = integer(bytes, 8);
    const auto actions = integer(bytes, 12);
    const std::uint32_t descriptor_size = selection ? 16 : title ? 12 : 8;
    if (!records || actions != (selection ? 22U : title ? 7U : 2U) ||
        std::uint64_t(records) * 8 + 16 + std::uint64_t(actions) * descriptor_size > bytes.size()) {
        invalid("invalid record or startup action count");
    }
    CharacterPack pack;
    pack.schema_ = selection ? Schema::Selection : title ? Schema::Title : Schema::Startup;
    std::size_t cursor = 16;
    for (std::uint32_t index = 0; index < actions; ++index) {
        if (selection) {
            pack.actions_.push_back({integer(bytes, cursor), integer(bytes, cursor + 4),
                                     integer(bytes, cursor + 8), integer(bytes, cursor + 12)});
        } else if (title) {
            pack.actions_.push_back({0, integer(bytes, cursor), integer(bytes, cursor + 4),
                                     integer(bytes, cursor + 8)});
        } else {
            pack.actions_.push_back({0, 0, integer(bytes, cursor), integer(bytes, cursor + 4)});
        }
        cursor += descriptor_size;
    }
    pack.records_.reserve(records);
    for (std::uint32_t index = 0; index < records; ++index) {
        if (bytes.size() - cursor < 8) {
            invalid("truncated record header " + std::to_string(index + 1));
        }
        const auto kind = integer(bytes, cursor);
        const auto length = integer(bytes, cursor + 4);
        cursor += 8;
        if (kind < 1 || kind > 14 || length > bytes.size() - cursor) {
            invalid("record " + std::to_string(index + 1) + " has invalid kind or truncated payload");
        }
        pack.records_.push_back({static_cast<ResourceKind>(kind),
                                 {bytes.begin() + cursor, bytes.begin() + cursor + length}});
        cursor += length;
    }
    if (cursor != bytes.size()) {
        invalid("trailing bytes after records");
    }
    pack.validate();
    return pack;
}

void CharacterPack::validate() const {
    std::vector<std::array<std::uint32_t, 3>> seen;
    for (const auto& action : actions_) {
        const std::array<std::uint32_t, 3> descriptor{action[0], action[1], action[2]};
        if (std::find(seen.begin(), seen.end(), descriptor) != seen.end()) {
            invalid("duplicate startup action descriptor");
        }
        const std::array<std::uint32_t, 2> title_descriptor{action[1], action[2]};
        const auto end = schema_ == Schema::Startup ? kTitleActionDescriptors.begin() + 2
                                                   : kTitleActionDescriptors.end();
        const bool title_action = action[0] == 0 &&
            std::find(kTitleActionDescriptors.begin(), end, title_descriptor) != end;
        const bool selection_action = schema_ == Schema::Selection &&
            std::find(kSelectionActionDescriptors.begin(), kSelectionActionDescriptors.end(),
                       descriptor) != kSelectionActionDescriptors.end();
        if (!title_action && !selection_action) {
            invalid("unknown startup action descriptor");
        }
        seen.push_back(descriptor);
        payload(*this, action[3], ResourceKind::Action, 8);
    }
    Validator validator(*this);
    validator.records();
    validator.graphs();
    validator.animation_parts();
}

const ResourceRecord& CharacterPack::record(ResourceId id) const {
    if (!id || id > records_.size()) {
        invalid("resource reference " + std::to_string(id) + " is out of range");
    }
    return records_[id - 1];
}
ActionHandle CharacterPack::action(StartupAction action) const {
    const auto index = static_cast<std::uint32_t>(action);
    for (const auto& entry : actions_) {
        if (entry[0] == 0 && entry[1] == 0 && entry[2] == index) {
            return ActionHandle{entry[3]};
        }
    }
    invalid("missing startup action descriptor " + std::to_string(index));
}
ActionHandle CharacterPack::action(TitleAction action) const {
    const auto index = static_cast<std::uint32_t>(action);
    if (index >= kTitleActionDescriptors.size()) {
        invalid("unknown title action descriptor");
    }
    for (const auto& entry : actions_) {
        if (entry[0] == 0 && entry[1] == kTitleActionDescriptors[index][0] &&
            entry[2] == kTitleActionDescriptors[index][1]) {
            return ActionHandle{entry[3]};
        }
    }
    invalid("missing title action descriptor " + std::to_string(index));
}
ActionHandle CharacterPack::action(SelectionAction action) const {
    const auto index = static_cast<std::uint32_t>(action);
    if (index >= kSelectionActionDescriptors.size()) {
        invalid("unknown selection action descriptor");
    }
    const auto& descriptor = kSelectionActionDescriptors[index];
    for (const auto& entry : actions_) {
        if (entry[0] == descriptor[0] && entry[1] == descriptor[1] && entry[2] == descriptor[2]) {
            return ActionHandle{entry[3]};
        }
    }
    invalid("missing selection action descriptor " + std::to_string(index));
}
ActionResource CharacterPack::resolve(ActionHandle handle) const {
    if (handle.identity > std::numeric_limits<ResourceId>::max()) {
        invalid("action handle is out of range");
    }
    const auto& bytes = payload(*this, static_cast<ResourceId>(handle.identity), ResourceKind::Action, 8);
    return {integer(bytes, 0), integer(bytes, 4)};
}
ObjectResource CharacterPack::object(ResourceId id) const {
    const auto& bytes = payload(*this, id, ResourceKind::Object, 52);
    ObjectResource value{integer(bytes, 0), integer(bytes, 4), {}, {}, {},
                         integer(bytes, 44), integer(bytes, 48)};
    for (std::size_t axis = 0; axis < 3; ++axis) {
        value.position[axis] = scalar(bytes, 8 + axis * 4);
        const auto bits = integer(bytes, 20 + axis * 4);
        std::memcpy(&value.rotation[axis], &bits, sizeof(bits));
        value.scale[axis] = scalar(bytes, 32 + axis * 4);
    }
    return value;
}
ModelResource CharacterPack::model(ResourceId id) const {
    const auto& bytes = payload(*this, id, ResourceKind::Model, 44);
    return {integer(bytes, 0), integer(bytes, 4), integer(bytes, 8), integer(bytes, 12),
            integer(bytes, 16), word(bytes, 20), word(bytes, 22),
            {scalar(bytes, 24), scalar(bytes, 28), scalar(bytes, 32)}, scalar(bytes, 36)};
}
MotionResource CharacterPack::motion(ResourceId id) const {
    const auto& bytes = payload(*this, id, ResourceKind::Motion, 16);
    return {integer(bytes, 0), integer(bytes, 4), word(bytes, 8), word(bytes, 10), integer(bytes, 12)};
}
} // namespace sadx::character_pack
