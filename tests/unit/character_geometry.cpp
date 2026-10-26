#include "engine/character_geometry.hpp"
#include <cassert>
#include <cstring>

int main(int argc, char **argv) {
    assert(argc == 2);
    using namespace sadx::character_pack;
    const auto pack = CharacterPack::load(argv[1]);
    GeometryState geometry(pack);
    std::size_t vectors = 0, indices = 0, coordinates = 0;
    for (std::size_t index = 1; index <= pack.record_count(); ++index) {
        const auto resource = static_cast<ResourceId>(index);
        const auto &record = pack.record(resource);
        const auto &bytes = record.payload;
        if (record.kind == ResourceKind::Vectors) {
            auto &values = geometry.vector_words(resource);
            assert(values.size() * 4 + 4 == bytes.size());
            assert(std::memcmp(values.data(), bytes.data() + 4, bytes.size() - 4) == 0);
            if (!values.empty()) {
                auto *identity = values.data();
                constexpr std::uint32_t kSignalingNan = 0x7fa12345;
                values[0] = kSignalingNan;
                assert(geometry.vector_words(resource).data() == identity);
                assert(geometry.vector_words(resource)[0] == kSignalingNan);
                std::uint32_t original;
                std::memcpy(&original, bytes.data() + 4, sizeof(original));
                assert(original != kSignalingNan);
            }
            ++vectors;
        } else if (record.kind == ResourceKind::Indices || record.kind == ResourceKind::Uv) {
            const auto &values = record.kind == ResourceKind::Indices
                                     ? geometry.indices(resource)
                                     : geometry.texture_coordinates(resource);
            assert(values.size() * 2 + 4 == bytes.size());
            assert(std::memcmp(values.data(), bytes.data() + 4, bytes.size() - 4) == 0);
            (record.kind == ResourceKind::Indices ? indices : coordinates)++;
        }
    }
    assert(vectors && indices && coordinates);
    bool wrong_kind = false, absent = false;
    try {
        (void)geometry.vector_words(1);
    } catch (const std::runtime_error &) {
        wrong_kind = true;
    }
    try {
        (void)geometry.indices(0);
    } catch (const std::runtime_error &) {
        absent = true;
    }
    assert(wrong_kind && absent);
}
