#include "engine/character_pack.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#include <type_traits>

int main(int argc, char **argv) {
    namespace c = sadx::character_pack;
    static_assert(!std::is_copy_constructible_v<c::ObjectState>);
    assert(argc == 2);
    auto pack = c::CharacterPack::load(argv[1]);
    c::ResourceId first = 0, other = 0, not_object = 0;
    for (c::ResourceId id = 1; id <= pack.record_count(); ++id) {
        if (pack.record(id).kind == c::ResourceKind::Object) {
            if (!first)
                first = id;
            else if (!other)
                other = id;
        } else if (!not_object)
            not_object = id;
    }
    assert(first && other && not_object);
    const auto imported = pack.object(first);
    c::ObjectState state(pack);
    auto *identity = &state.object(first);
    constexpr float kChangedPosition = 123.25f;
    identity->position[0] = kChangedPosition;
    identity->child = other;
    for (c::ResourceId id = 1; id <= pack.record_count(); ++id)
        if (pack.record(id).kind == c::ResourceKind::Object)
            static_cast<void>(state.object(id));
    const auto &const_state = state;
    assert(&const_state.object(first) == identity);
    assert(state.object(first).position[0] == kChangedPosition);
    assert(state.object(first).child == other);
    assert(pack.object(first).position == imported.position);
    assert(pack.object(first).child == imported.child);
    for (auto bad : {c::ResourceId{0}, not_object, std::numeric_limits<c::ResourceId>::max()}) {
        bool failed = false;
        try {
            static_cast<void>(state.object(bad));
        } catch (const std::runtime_error &) {
            failed = true;
        }
        assert(failed);
    }
    std::cout << "Character objects: shared mutable identity, stable borrows, immutable import "
                 "and missing-resource boundaries pass\n";
}
