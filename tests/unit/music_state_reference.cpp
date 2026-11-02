#include "guest/music_state_reference.hpp"
#include "guest/game_reset_reference.hpp"
#include <cassert>
#include <cstring>

namespace sadx::reference {
struct MusicTracksTestAccess {
    static music::TrackState &storage(MusicTracks &owner) { return owner.tracks_; }
};
} // namespace sadx::reference

namespace {
using namespace sadx;
using reference::TrackWords;
struct Fixture {
    std::optional<std::uint32_t> depth, queue, initialized;
    std::optional<std::int32_t> timing;
    std::array<std::optional<std::uint32_t>, 36> records{};
    ResetLeafState leaves{{}, {}, depth, queue, {}, {}, timing, {}, records, {}, {}, initialized,
                          {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}};
};

void availability_masks() {
    const TrackWords values{0x80000000U, 0xffffffffU, 0x87654321U,
                            0xfedcba98U, 0x81234567U, 0xf1234567U};
    for (unsigned mask = 0; mask < 64; ++mask) {
        for (unsigned resets = 0; resets < 4; ++resets) {
            Fixture fixture;
            reference::MusicTracks owner;
            auto words = values;
            for (unsigned i = 0; i < 6; ++i)
                if (!(mask & (1U << i)))
                    words[i].reset();
            owner.import_words(words);
            assert(owner.export_words() == words);
            auto expected = words;
            unsigned calls = 0;
            const ResetLeafNested nested = [&](const ResetLeafCall &) {
                ++calls;
                return GameLogicOwnerResult{GameLogicOwnerState::failed, 7, {}};
            };
            for (unsigned family = 0; family < 2; ++family) {
                if (!(resets & (1U << family)))
                    continue;
                const auto result = owner.dispatch(family == 0 ? 0x4257c0 : 0x4257e0,
                                                    {}, fixture.leaves, nested);
                assert(result.outcome == ResetLeafOutcome::completed);
                assert(result.source_boundary == 0 && !result.eax);
                assert(result.nested.state == GameLogicOwnerResult{}.state);
                if (family == 0) {
                    expected[0] = expected[1] = expected[2] = 0xffffffffU;
                    expected[4] = 0;
                } else {
                    expected[3] = 0xffffffffU;
                    expected[5] = 0;
                }
                assert(owner.export_words() == expected);
            }
            assert(calls == 0);
            // Reimport absence removes knowledge, without overwriting the live native owner.
            const auto live = reference::MusicTracksTestAccess::storage(owner);
            owner.import_words({});
            assert(owner.export_words() == TrackWords{});
            assert(reference::MusicTracksTestAccess::storage(owner).current_song == live.current_song);
            assert(reference::MusicTracksTestAccess::storage(owner).current_voice == live.current_voice);
            assert(reference::MusicTracksTestAccess::storage(owner).real_previous_song == live.real_previous_song);
            assert(reference::MusicTracksTestAccess::storage(owner).previous_song == live.previous_song);
            assert(reference::MusicTracksTestAccess::storage(owner).song_queue_state == live.song_queue_state);
            assert(reference::MusicTracksTestAccess::storage(owner).voice_queue_state == live.voice_queue_state);
            owner.import_words(values);
            assert(owner.export_words() == values);
            reference::MusicTracksTestAccess::storage(owner).current_song = 42;
            reference::MusicTracksTestAccess::storage(owner).voice_queue_state = 99;
            auto changed = values;
            changed[0] = 42;
            changed[5] = 99;
            assert(owner.export_words() == changed); // no snapshot copyback
        }
    }
}

void source_mapping_and_legacy_dispatch() {
    constexpr std::array<std::uint32_t, 6> addresses{
        0x912698, 0x91269c, 0x9126a8, 0x912694, 0x3b29cf8, 0x3b29cf4};
    reference::MusicTracks owner;
    unsigned read = 0, written = 0;
    owner.import_source([&](std::uint32_t at) -> std::optional<std::uint32_t> {
        assert(at == addresses[read]);
        return 0x80000000U + read++;
    });
    owner.export_source([&](std::uint32_t at, std::optional<std::uint32_t> value) {
        assert(at == addresses[written] && value == 0x80000000U + written);
        ++written;
    });
    assert(read == 6 && written == 6);
    Fixture fixture;
    auto result = owner.dispatch(0x123456, {}, fixture.leaves, {});
    assert(result.outcome == ResetLeafOutcome::not_a_leaf && !result.eax);
    result = owner.dispatch(0x401410, 17, fixture.leaves, {});
    assert(result.outcome == ResetLeafOutcome::completed && fixture.depth == 17 && !result.eax);
    fixture.initialized = 1;
    result = owner.dispatch(0x410100, 0, fixture.leaves, {});
    assert(result.outcome == ResetLeafOutcome::nested_failed);
    assert(result.source_boundary == 0x41010d && !result.eax);
    fixture.leaves.word_at_389d608 = 0;
    fixture.leaves.entry_words_at_3d08428[0] = 3;
    result = owner.dispatch(0x785330, 0, fixture.leaves, {});
    assert(result.outcome == ResetLeafOutcome::completed && result.eax == 0xffffffffU);
}

void reset_failure_prefixes() {
    for (const bool fail_before_tracks : {true, false}) {
        Fixture fixture;
        reference::MusicTracks owner;
        const TrackWords initial{11, 12, 13, 14, 15, 16};
        owner.import_words(initial);
        reference::FrameMessageFact status = 7;
        CrtRandState rng{5};
        GameResetState state{status, 3, 0, 0, rng};
        GameResetRuntime runtime{state, [&](const GameResetCall &call) {
            if (call.callee == (fail_before_tracks ? 0x401410U : 0x424460U))
                return GameLogicOwnerResult{GameLogicOwnerState::failed, call.source_call, {}};
            if (call.callee == 0x4257c0 || call.callee == 0x4257e0) {
                const auto result = owner.dispatch(call.callee, call.argument, fixture.leaves, {});
                assert(result.outcome == ResetLeafOutcome::completed && !result.eax);
            }
            return GameLogicOwnerResult{GameLogicOwnerState::completed, 0, {}};
        }};
        assert(!run_game_reset_40bf40(runtime));
        const TrackWords reset{0xffffffffU, 0xffffffffU, 0xffffffffU, 0xffffffffU, 0, 0};
        assert(owner.export_words() == (fail_before_tracks ? initial : reset));
    }
}
} // namespace

int main() {
    availability_masks();
    source_mapping_and_legacy_dispatch();
    reset_failure_prefixes();
}
