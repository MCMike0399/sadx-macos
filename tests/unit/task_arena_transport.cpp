#include "guest/task_reference.hpp"
#include "engine/task_work.hpp"

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

using namespace sadx;

int main() {
    TaskWorkArena arena(sadx::task_reference::pointer_encoder(0x20000000));
    auto mirror = arena.source_bytes();
    std::vector<unsigned> events;
    arena.set_metadata_writer([&](unsigned at, std::optional<unsigned> value) {
        assert(value);
        events.push_back(1);
        std::memcpy(mirror.data() + at, &*value, 4);
    });
    arena.set_payload_transport([&](unsigned at, void* out, unsigned bytes) {
        events.push_back(2);
        std::memcpy(out, mirror.data() + at, bytes);
        return true;
    }, [&](unsigned at, const void* in, unsigned bytes) {
        events.push_back(3);
        std::memcpy(mirror.data() + at, in, bytes);
        return true;
    });
    auto owner = arena.allocate(512).work;
    auto target = arena.allocate(64).work;
    auto other = arena.allocate(64).work;
    events.clear();
    assert(arena.set_reference_view(owner, 0x138, owner, 0x38));
    assert(arena.set_reference(owner, 0x13c, owner));
    assert((events == std::vector<unsigned>{1, 1})); // metadata bypasses payload writes
    auto self = arena.reference_view(owner, 0x138);
    assert(self && self.work == owner && self.byte_offset == 0x38);
    assert(self.allocation_data_offset == owner->data_offset());
    assert(arena.set_reference_view(owner, 0, target, 12));
    const auto original_offset = target->data_offset();
    unsigned raw = 0;
    std::memcpy(mirror.data() + owner->data_offset(), &raw, 4);
    events.clear();
    const auto null = arena.reference_view(owner, 0);
    assert(null && !null.work && !null.allocation_data_offset);
    assert((events == std::vector<unsigned>{2})); // live zero overrides cached target
    unsigned resolutions = 0;
    arena.set_reference_resolver([&](unsigned observed) {
        ++resolutions;
        events.push_back(4);
        assert(observed == 0x20000000 + other->data_offset() + 7);
        return TaskWorkReferenceResult{TaskWorkFailure::None, other, 7, other->data_offset()};
    });
    raw = 0x20000000 + other->data_offset() + 7;
    std::memcpy(mirror.data() + owner->data_offset(), &raw, 4);
    events.clear();
    const auto redirected = arena.reference_view(owner, 0);
    assert(redirected && redirected.work == other && redirected.byte_offset == 7);
    assert((events == std::vector<unsigned>{2, 4}) && resolutions == 1);
    raw = 0x20000000 + original_offset + 12;
    std::memcpy(mirror.data() + owner->data_offset(), &raw, 4);
    assert(arena.release(target));
    auto reused = arena.allocate(64).work;
    assert(reused != target && reused->data_offset() == original_offset);
    {
        const auto released = arena.reference_view(owner, 0);
        assert(released.failure == TaskWorkFailure::ReleasedWork && released.work == target);
        assert(released.byte_offset == 12 && released.allocation_data_offset == original_offset);
    }
    assert(resolutions == 1); // same raw address still names the released generation
    target.reset();
    const auto expired = arena.reference_view(owner, 0);
    assert(expired.failure == TaskWorkFailure::ReleasedWork && !expired.work);
    assert(expired.byte_offset == 12 && expired.allocation_data_offset == original_offset);
    assert(resolutions == 1);
    std::cout << "Arena transport: metadata/payload ordering, live null/redirect, interior/self "
                 "aliases, released generation and saved allocation offset passed.\n";
}
