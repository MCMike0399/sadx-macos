# Music scheduling and reference track reset owner

Ready for Root's independent review, not integration approval or a gate verdict.

Worktree: `/Users/burbujamc/.t3/worktrees/sadx-macos/t3-d8ad865d`.
Branch: `packet/music-state-reset-routing`; immutable Main base `73be9a5`.

Commits, in order:

1. `21db0f1`: context, initial source bindings, SDK lookup and recovered historical fault intent.
2. `7088654`: fresh baseline, exact partial/boundary identities, actual source instruction/extent evidence and pre-edit qualification of 24 historical faults.
3. `8a37d10`: canonical owner and both native/reference consumer changes.
4. `622a525`: separate evidenced probe bindings, build dependencies, source fingerprints, retired spelling allowances, capture tool and two new characterization tests.
5. The following qualification commit: receipts, source hashes, limits and Root steps in this packet directory.

`QUALIFICATION.json` binds all 13 changed source files and records every check and limit.
`BASELINE.json` binds complete local output streams. `composed-reset-identities.json`
records all 300 completed/boundary/page/trace output identities;
`title-partial-identities.json` records all 270 source/native FP differences.
The original judge ASTs are identical after normalizing only their maintained-source
fingerprint lists (`unchanged-judge-logic.json`). No original vectors, comparisons,
expected values, seeds, tolerances or failure indices changed.

The actual ownership declaration is in `native_game_reference.hpp` in this base;
there is no separate `ownership.hpp`. It now owns one reference `MusicTracks` with
one typed TrackState and six availability flags. No reference timing/task producer
was added. Native Session owns SchedulingState at the old manager slot. Moving the
POD timing has no callbacks; the manager retains its relative destruction placement.
PCM still borrows `sound_.stream_initialized`. Character, regular/menu/GPU, banks,
media/player/readers/config and scene request owners are unchanged.

The two legacy reset bodies are removed. Reference dispatch recognizes only the
song/voice reset entries and invokes the unchanged cleanup implementation before
marking exactly four/two fields known. Other entries still delegate to the existing
legacy interpreter. No game-tier include or call points upward into the adapter.
Manager initialization retains its six-store body; no reset-pair reuse was attempted.
Manager deletion retains five stores and preserves real_previous_song.

All seven captured native streams match baseline byte for byte. Default original
music has 1,301 observations: 1,299 full executions and two duration[-1] bounded
prefixes. Menu voice 4,888, cleanup 2,604 (including its nested reset judge), standalone
reset 300, composed reset 300 and stream players 600 retain their outcomes. Title
has 20,872 cases, zero value/call mismatches and the same 270 FP partial identities;
its exit 1 remains a partial result. Standalone reset captures the same 300 cases
already nested in cleanup; these are not extra unique coverage.

Six focused units compile/run 0: music_state_reference, music_scheduling_state,
music, menu_voice, game_cleanup and task_callback_lifetime. The new adapter test
covers all 64 availability masks with no reset, song-only, voice-only and both;
high-bit words, known-to-unknown reimport, no default leakage, live exports, legacy
optional-EAX/boundary results and earlier/later reset failures. Scheduling tests
exercise initializer/destructor asymmetry, live handles and allocation/publication.
All 24 historical faults were compiled/caught before source edits and reproduce
identical mismatch counts after the change. All 12 additional ownership/order
faults compile and abort the relevant assertion; compiler failures never count as
caught. Specs and qualified results are in the two mutant JSON receipts.

A maintained-source scheduling test also compiles and runs without any guest
adapter, maintained game archive or executable. This is a bounded native slice;
product archive/app/force-load closure remains Root's work. NativeGame plus all
three changed runtime consumers pass private syntax compilation. Layer, ownership,
provenance and readability checks pass. Profile checking over 75 declared original
ranges reports zero unhandled overlaps. Constexpr checking still fails on two
inherited player_rotation.cpp fingerprints from Root's separate length work. Only
our diagnostic names consumer hash was revalidated, after reversing the three
subobject spellings proved the entire file identical to base.

Private artifacts remain under `build/music-owner/{baseline,current,qualification}`.
The full capture tool is `tools/analysis/capture_music_owner.py`; run it with Root's
pinned Python, `--libraries DIR --output NEW_DIR --exe PATH`. The libraries must be
fresh builds of the measured source cohort. Output `.bin` streams include every
native array word and trace byte, plus actual non-void C returns. Partial identities
come from the unchanged original source results and the native FP word. Compiler
commands, logs and individual mutation reports are hashed in `local-receipts.json`.
Private link setup failures are retained: crt_integer dependency, duplicate reset
export, and title frame-loop/speedrun dependencies. They are not maintained build
defects. The existing registered Sonic forced include was not modified.

Root steps:

1. Independently review this exact packet and source evidence; run reserved holdout.
2. Merge character-resource Session changes additively. Keep music owner placement,
   canonical manager handles, PCM borrow and scene voice/music task distinction.
3. Add the no-argument execution rows `music-state-reference` and
   `music-scheduling-state` to `tests/unit/manifest.tsv`. The supplied Makefile
   already supplies the guest archive dependency for the reference unit/probe.
4. Union protected judge changes while preserving every inherited path and all
   53 ignored corpora. The tool-created checkout did not contain those 53 files;
   no manifest was refreshed, narrowed or widened here. Their expected identities
   and the unchanged 938-path manifest hash are in `preserved-protected-inputs.json`.
5. Rebuild native product/force-load closure, run actual menu/save journeys and
   the coherent composite gate. Root owns inventory, STATUS and HANDOFF closure.

No full gate, reserved holdout, product closure, journeys, delegates, downloads,
provider/network/host/storage operations, Main writes or push were performed.
No Windows/device/codec/thread/race/performance, whole-audio/Session or campaign
completion claim. The other legacy reset/media/player/server cases remain debt
in place. Canonical memory outside this isolated checkout was not edited.
