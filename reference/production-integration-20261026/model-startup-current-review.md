# Startup and current-player owner review

A: **bounded approval of `88eb56e7eca1ec558afa7f2729d86740f9ead81a`; no blocking findings.**
B: **bounded approval of `28b3974d53ad30ebc7be49582b4b39cac950aaf6`; no blocking findings.**
Neither verdict is a full gate, N3b, actor-pixel, GPU or gameplay qualification.

Reviewed A as `83f0ded..88eb56e`, especially `004d03f`, `a316c9a` and final compiler
registrations. Read the prior `635df95` binding review and `afc50a7` initializer review,
plus their committed source/proof/contract records. Reviewed B against its recorded base
`c29d687a319d30ea986ea7ad2b0795751e8e7a7d`, including PACKET, code, facade and proof.
AGENTS/CLAUDE, GOAL, STYLE section 12, STATUS, HANDOFF and relevant initialization,
model/strip and texture-lifetime memory informed the review. Standards and spec were
checked independently within this review; no additional workers were started.

Source was read through frozen git objects. Existing ignored producer receipts were read
and hashed, never executed. Only REVIEW.md and REVIEW-evidence.json were authored on
`packet/review-startup-current` in the assigned review worktree. **Tests NOT RERUN.**
No builds, apps, Ghidra, gate, broad tests, source-checkout mutations or Root writes occurred.

## A — Standards

No actionable standards finding; no required fix in this bounded diff.

- STYLE sections 1, 4–6 and 12: actual native Context remains one Display reference;
  engine state is semantic, source ownership is canonical, and native bindings use direct
  typed calls. No fixture availability, guest layout, dynamic lookup, copied numeric owner,
  new manager/registry or game-tier address dispatch was introduced.
- The six initializer/owner implementation files enumerated in REVIEW-evidence.json are
  byte-identical to approved `bdf110a`. Their reference fact/conversion implementation is
  separate from native Context/resource definitions. Existing ownership recovery is reused,
  rather than inferred from a reduction in the readability baseline.
- `Makefile:443–455` puts the compatibility include directly on the named legacy fixture
  compilation recipes. No global/inherited CXXFLAGS injection reaches game TUs. These
  targets link Guest then Core; the native source list includes shared engine/native
  bindings and maintained library defaults, excluding guest adapters.
- The initialization checker whitespace change has independently identical Python ASTs
  between `bdf110a` and this freeze. All 12 legacy source files listed in the registration
  receipt are byte-unchanged from `83f0ded`. No old expected value, tolerance or fixture
  was relaxed. The coexistence option changes only inclusion of now-conflicting throwing
  test hooks; both reference and native execution paths remain in the executable.

## A — Spec and fidelity

No actionable fidelity finding; no required fix in the bounded startup/binding path.

1. **Correct null-texture callee.** `src/runtime/model_draw.cpp:81` and
   `src/runtime/model_draw_context.hpp:11` call existing `unbind_cached_texture` through
   borrowed Display lifetime state/services. `Display::texture_state` returns references
   to the real pool/cache/active/list/scratch owners. Existing helper 78bba0 unbinds first,
   then clears cache and active only when cache differs from -1. LogoRenderer unbind drops
   its weak surface observation and device image, without releasing loaded pool references.
   This differs materially from Display.clear_texture_binding / 77fc50 release-all.
   The contrary claim in the prior 635df95 review is superseded. The new unit at
   `tests/unit/model_binding.cpp:39` verifies retained references/surface/image and the
   cache=-1 stale-active/image guard. This is a real adapter correction, not a judge fix.
2. **Actual count producer and borrowed identity.** `native_display.cpp:45–62` copies all
   nine maintained configuration counts into this Display's existing library::State before
   shared initialization. Record 8 therefore uses configured 32000, not PE default 12000.
   Windowed/backbuffer fields use maintained defaults; fixed actual viewport 640x480 yields
   exact aspect 0.75 using the actual arithmetic context. `model_device.hpp:23` and
   `polybuff_initialization_binding.cpp:4` borrow/reread that same live count array per
   record. No per-draw cap, cached second count array or fixture owner substitutes for it.
3. **Complete resources and stable lifetime.** `native_display.hpp:118–126` owns one
   canonical nine-record array, giant scratch, retained base and 128-word clear block.
   Actual shared InitPolyBuffers receives ignored 32 and 24576 scratch vertices, initializes
   all nine records through recovered PolyBuff_Init and clears all 128 words. The native RAM
   allocator returns null on bad_alloc; existing per-record allocation-failure behavior is
   retained, with no successful fallback or invented rollback. Normal host teardown at
   `native_display.cpp:78` clears the device stream reference before freeing all nine game
   handles/argument blocks, then deletes giant scratch. Context borrows remain synchronous;
   Display is not implicitly copyable/movable. Submitted Draw values already own snapshots.
4. **Workspace identity only.** `native_display.hpp:125` is an explicitly opaque private
   WorkspaceIdentity whose address is retained by the shared initializer. Inspected runtime
   uses neither dereference it nor infer contents/extent; the engine merely stores base.
   It is adequate for this retained-identity relation. No current native byte consumer or
   required value relation was found that makes this a blocker. This approval establishes
   no workspace byte values, allocation extent or future direct-workspace implementation.
   Likewise giant scratch allocation size does not prove source-initialized byte contents.
5. **Actual consumption.** `model_buffer_seam.cpp:5` now returns canonical record 8 through
   actual Context. `native_display.cpp:273` discards all nine actual buffers through the
   recovered helper each frame. The composed unit removes its fake buffer/getter, constructs
   actual Display and checks resources, count identity/live mutation, scratch size/clear,
   configured capacity and frame discard before real Question/material/environment/transform/
   texture/strip/device composition. Existing immutable-prior-Draw/shared-geometry assertions
   remain. This establishes composed CPU behavior, not all preview actors or GPU delivery.

## A — Existing evidence and limits

- Five binding mutants have compiled exit 0 and assertion exit -6. All five retained mutant
  source hashes match the committed receipt. These cover draw/retention/material/vector/
  matrix bindings; they are not five new startup-specific mutation experiments.
- Actual native/reference coexistence receipt reports exit 0 and nine reference calls plus
  actual native device unit execution. Its private copied unit exactly matches the frozen
  unit with only main renamed and explicit return 0 added. Context/unit hashes match the
  freeze. The inspected link command uses ordinary static linkage, no dynamic lookup;
  reference symbols remain in polybuff_reference. Actual Display construction now executes.
- Existing focused reports contain 600 system, 2000 device and 3969 frame cases with empty
  mismatch lists; current initialization integration has 152 cases, passed true, zero value
  mismatches. This 152-case receipt is distinct from the prior packet's larger qualification.
- Retained fixture-unit receipt has ninja-view exit 1 from missing required trig argument;
  the separately retained corrected invocation exit is 0. Other listed units are 0. The
  initial usage failure is not counted as a successful run.
- Source-only build exit is 0. All 618 inspected source/build-input paths in its export
  match frozen 88eb56e; the retained build log compiles native objects and links the native
  archive without guest adapters or original game objects. Only pinned platform dependency reuse is recorded. Its completed qualification exit.json is 1; application.log
  stops at native_gamma_player_controller at frame 1382. This is an honest next frontier,
  not successful N3b completion. No image was newly inspected in this review, and no actor
  pixels are inferred from either the unit or this frontier.
- No full gate includes A. Prior approved algorithm FP partials remain partial; no new
  arithmetic oracle, holdout or mutation run was performed by this reviewer. Full combined
  gate and broader journey/GPU qualification remain Root work.

## B — Standards

No actionable standards finding; no required fix in the bounded owner migration.

- `src/player/actor_current.hpp:8–14` gives the three source addresses one plain State owner
  each, with source BSS zero defaults and semantic fields. gpCharTwp/gpCharPwp are distinct
  from registries; SuperSonicFlag is 3c55d45, not 3b18db4 or Metal's 3b18db5. Source extents
  describe the original image, not the host shared_ptr layout. Section metadata in the proof
  places these spans beyond raw .data bytes and inside its zero-filled virtual tail.
- `sonic_player.hpp:66` owns that same State type once; old value fields and six spelling
  allowances are removed. New helpers are NOTSA and add no virtual service, manager,
  registry, fixture fields or arithmetic. Producer readability receipts report new-file
  debt zero; the baseline adds only zero metrics and decreases old identifier allowances.
- The reference facade is confined to tests/reference and the documented fixture-TU-only
  compile proposal. It adds availability booleans and const references, not another numeric
  owner. Root must still register/link the source and set up those precise fixture recipes;
  no approval here extends to an unreviewed setup or Gamma/Session binding.

## B — Spec and fidelity

No actionable fidelity finding; no required fix in the bounded publication path.

1. `actor_current.cpp:4–20` publishes authoritative state before optional observation.
   Null, same-value and all byte values are preserved. Empty/missing callbacks do not veto
   stores; callback exceptions propagate only after publication. Readers borrow the actual
   handle slots or return the exact byte. No bool normalization or automatic registry refresh
   was added, so stale/null PWP remains until a source publication.
2. `sonic_player.cpp:1520–1525,1605` replaces exactly the old primary, player and super-byte
   stores at their existing positions. The existing ternary/null behavior and super range
   predicate remain. Algorithms, call ordering and arithmetic otherwise stay unchanged.
   Gamma may borrow primary/player references from this State; it must not acquire a
   duplicate owner, refresh player implicitly, or update the super byte without source proof.
3. `tests/reference/actor_current_fixture.hpp:8–27` has one const-reference value per receipt,
   pointing into inherited canonical current state. Observer callbacks set only availability.
   Default construction installs callbacks on this exact instance after member construction;
   deleted copy operations suppress implicit moves as well, preventing moved self-references.
   Frozen fixtures use the local facade for outputs; production runtime retains base Globals.
   Receipt has_value/value_or therefore preserve prior write-availability semantics without
   replacing stored values or manufacturing zero/null publications.
4. The new unit checks actual identity and mutable borrowing, stale player state, authoritative
   null publication, observer/store order, same-value stores, empty observers, exception-after-
   store, raw byte preservation and independent worlds. Its 257-world loop covers byte values
   0..255 plus wraparound. It introduces no new recovered source-range claim.

## B — Existing evidence and limits

All 13 frozen judges match their proof hashes and are byte-unchanged from c29d687. All 59
combined file-hash comparisons in this review match, including B's source registrations and
both oracle source lists and A's coexistence inputs. The four retained B raw oracle reports
independently match their recorded full-file hashes and preserve case digests:

- Player baseline/final: 28,800 cases, zero stored mismatches, 2,840 FP differences;
  ground-prefix subset 4,032 / zero values / 1,632 FP differences.
- Actions baseline/final: 30,192 cases, zero stored/boundary mismatches, 2,172 FP differences.

Both remain **partial, passed=false** despite legacy verifier rc 0. The three unchanged
legacy units, core-only unit and standalone actor_current + task_work linkage report 0;
raw unit output retains the 136 ECoast paths/96 responses. Seven compiled mutants report
assertion failures; reconstructing every exact source replacement yields its recorded hash.
Profile receipt reports five unchanged Player ranges, zero UNHANDLED overlaps; Actions has
no addressed ranges. No new recovered range is implied. Provenance/owners/layers/readability
receipts were inspected, not rerun. The existing legacy ground-acceleration guest linkage
remains excluded from product qualification.

B is approved as an owner/publication packet for Root's subsequent setup and native/Gamma
consumption. It does not qualify SourceActorCore, a complete Sonic/Gamma controller, native
actor output, GPU, N3b or gameplay. Any changed-code recheck requires a new Root delegate
with the full prior context; this review's verdicts attach only to the frozen commits above.
