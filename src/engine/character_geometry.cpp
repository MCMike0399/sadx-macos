#include "engine/character_geometry.hpp"
#include <stdexcept>

namespace sadx::character_pack {
namespace {
constexpr std::size_t kArrayHeaderBytes = 4;
constexpr std::size_t kComponentBytes = 4;
constexpr std::size_t kIndexBytes = 2;
constexpr std::int32_t kSignedShortLimit = 1 << 15;
constexpr std::int32_t kUnsignedShortRange = 1 << 16;
// NOTSA: serialized little-endian arrays become real typed objects, without aliasing byte storage.
std::uint32_t integer(const std::vector<std::uint8_t> &bytes, std::size_t offset,
                      std::size_t width) {
    std::uint32_t result = 0;
    for (std::size_t index = 0; index < width; ++index)
        result |= std::uint32_t(bytes.at(offset + index)) << (index * 8);
    return result;
}
// NOTSA: type errors are rejected at the imported resource boundary.
void require_kind(const CharacterPack &pack, ResourceId resource, ResourceKind kind) {
    if (pack.record(resource).kind != kind)
        throw std::runtime_error("character geometry: wrong resource kind");
}
} // namespace

MaterialState::MaterialState(const CharacterPack &pack)
    : pack_(pack), arrays_(pack.record_count() + 1) {
    constexpr std::size_t kMaterialBytes = 20;
    for (std::size_t index = 1; index <= pack.record_count(); ++index) {
        const auto &record = pack.record(static_cast<ResourceId>(index));
        if (record.kind != ResourceKind::Materials)
            continue;
        auto &array = arrays_[index];
        const auto count = integer(record.payload, 0, kComponentBytes);
        array.resize(count);
        for (std::size_t item = 0; item < count; ++item)
            for (std::size_t field = 0; field < array[item].words.size(); ++field)
                array[item].words[field] = integer(record.payload,
                    kArrayHeaderBytes + item * kMaterialBytes + field * kComponentBytes,
                    kComponentBytes);
    }
}
SceneMaterial &MaterialState::material(ResourceId resource, std::size_t index) {
    require_kind(pack_, resource, ResourceKind::Materials);
    return arrays_.at(resource).at(index);
}
const SceneMaterial &MaterialState::material(ResourceId resource, std::size_t index) const {
    require_kind(pack_, resource, ResourceKind::Materials);
    return arrays_.at(resource).at(index);
}

GeometryState::GeometryState(const CharacterPack &pack)
    : pack_(pack), vectors_(pack.record_count() + 1), shorts_(pack.record_count() + 1) {
    for (std::size_t index = 1; index <= pack.record_count(); ++index) {
        const auto &record = pack.record(static_cast<ResourceId>(index));
        const auto &bytes = record.payload;
        if (record.kind == ResourceKind::Vectors || record.kind == ResourceKind::Colors) {
            auto &values = vectors_[index];
            for (std::size_t offset = kArrayHeaderBytes; offset < bytes.size();
                 offset += kComponentBytes)
                values.push_back(integer(bytes, offset, kComponentBytes));
        } else if (record.kind == ResourceKind::Indices || record.kind == ResourceKind::Uv) {
            auto &values = shorts_[index];
            for (std::size_t offset = kArrayHeaderBytes; offset < bytes.size();
                 offset += kIndexBytes) {
                const auto bits = static_cast<std::int32_t>(integer(bytes, offset, kIndexBytes));
                const auto value = bits < kSignedShortLimit ? bits : bits - kUnsignedShortRange;
                values.push_back(static_cast<std::int16_t>(value));
            }
        }
    }
}
std::vector<std::uint32_t> &GeometryState::vector_words(ResourceId resource) {
    require_kind(pack_, resource, ResourceKind::Vectors);
    return vectors_.at(resource);
}
const std::vector<std::uint32_t> &GeometryState::vector_words(ResourceId resource) const {
    require_kind(pack_, resource, ResourceKind::Vectors);
    return vectors_.at(resource);
}
std::vector<std::uint32_t> &GeometryState::color_words(ResourceId resource) {
    require_kind(pack_, resource, ResourceKind::Colors);
    return vectors_.at(resource);
}
const std::vector<std::uint32_t> &GeometryState::color_words(ResourceId resource) const {
    require_kind(pack_, resource, ResourceKind::Colors);
    return vectors_.at(resource);
}
const std::vector<std::int16_t> &GeometryState::indices(ResourceId resource) const {
    require_kind(pack_, resource, ResourceKind::Indices);
    return shorts_.at(resource);
}
const std::vector<std::int16_t> &GeometryState::texture_coordinates(ResourceId resource) const {
    require_kind(pack_, resource, ResourceKind::Uv);
    return shorts_.at(resource);
}
} // namespace sadx::character_pack
