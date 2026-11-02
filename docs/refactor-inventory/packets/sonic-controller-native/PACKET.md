# Sonic controller native source packet

Base `ca640c5`; branch `packet/sonic-controller-native`. Root approved the local
borrowing contract after context commit `0672b82`. This continuation implements
one semantic controller body with separate native and reference bindings.
Game-source candidate: `c583076`, correcting P1 review `bab7168d` atop Root’s
annotation-only correction `c3998a4`. Earlier qualification remains historical.
Judge/consumer migrations: `3cbdd10`, `6676119` and `29ecccc`.
Independent review, Session integration and the coherent gate remain Root-owned.

## Source and interfaces

* `player/sonic_player_internal.hpp` contains all recovered predicates, action
  checks, controller modes, ground heading, local-speed/floor and collider-center
  algorithms. Named modes, masks, compiler literals and ExactFloat expressions
  replace address dispatch and emulator arithmetic. ExactFloat retains all twelve
  historical CW observations; no FP partial is promoted to pass.
* `player/sonic_player.hpp`, `sonic_player_sites.hpp` and `sonic_player_seams.hpp`
  define semantic modes, per-consumption diagnostics and typed calls. Native
  failures are named operations. Only the reference adapter maps sites to PCs or
  marshals source call arguments.
* `runtime/native_sonic_context.hpp` is the Root-facing callback contract. It
  borrows PlayerSystem/current/registry, scheduler/arena and actual task. Player
  resolution uses the actual motion reference and canonical allocation map;
  initialization calls the existing canonical initializer. No player mirror or
  second registry is created. `native_sonic_fields.hpp` borrows legacy aggregate
  fields by typed structured bindings with semantic names: no byte offsets,
  independent values or eager validation. Schema additions fail compilation.
* `runtime/native_sonic_binding.cpp` resolves canonical collision components and
  live actor links. Ground acceleration calls `physics::calculate_ground_acceleration`,
  not guest acceleration. Air/speed/math/animation use maintained owners. Unbound
  hold/position/rotation/sound/model/common-tail compositions fail explicitly.
* `guest/sonic_player_reference.*` retains the legacy ABI and lazy missing-fact
  boundaries, nested results, dispatch observations and partial writes. The
  NativeGame header includes that reference contract. Both paths instantiate the
  same Sonic body. No recovered mode was removed or recovered gameplay relocated
  into guest code.

## Event link correction from Root's integration note

Cached Ghidra evidence `ev_9faf4e353760c9f6ed29c53231f3b3aa7417e4a33fc9b8398c208b0051a3505e`
for 495fa0 shows `495fc7: CMP dword ptr [EAX + 0x4c], EBX`.
The pinned New.h layout puts action.list at 0x40 and path.list at 0x4c.
The legacy field named action_list_at_4c is therefore not evidence for preview's
separate action-tail owner. Its ABI and all reference inputs remain unchanged.
The semantic body calls a neutral event-link read. Native Context exposes
read_event_mode/write_event_mode/read_event_link callbacks; absence is a named
stop. It never projects preview's tail or the old field into that native seam.
The controlled harness proves that even a populated legacy field cannot supply it.

## Evidence and checks

`BASELINE.json` and both original partial-identity files remain unchanged. Complete
ignored captures remain in this worktree. Historical pre-review receipts are in `VERIFICATION.json`; the corrected source
is qualified in `PHYSICS-CAPTURE-VERIFICATION.json`. The preserved baseline has: 58,992 complete old/current ABI
records match, plus two source-backed adequacy cases and 3,072 fresh extra records.
Default FP partials remain 2,840 and 2,172; extra cohorts retain 204 and 219.
Seven focused programs and five collector negatives pass; six faults compile and
are caught. Profile coverage has all 25 recovered entry bodies, zero UNHANDLED. Initial two faults survived because the
old defaults lacked damage bit four and speeds between the tested thresholds.
Two additional original-backed cases strengthen those observations without
changing or removing any default case; one retains its source PE flag partial.
All collector/mutant/holdout definitions are in `tools/oracles/sonic_controller`.
New collector negative tests cover empty/truncated/reordered/duplicated captures,
missing output words, baseline-anchor drift, every word and return comparison.

## Exact Root integration requirements

1. Apply both the separate reference-include migration `3cbdd10` and the source
   commits. Existing judge bodies changed only include directives; Root must
   reconcile those protected hashes in a separate evidenced judge integration.
2. Keep canonical owners unchanged. Construct sonic::Context in the real Sonic
   callback from Session's owners; call sonic::run. Supply only reached actual
   stage/model/initialization/animation/input owners. Missing editor/event readers
   remain named stops. Neither preserved reset a27ef40 nor its ABI/vectors is used.
3. Add `src/player/sonic_player.cpp` and the required maintained native dependency
   closure to native-sources.txt. Native binding .cpp is picked up by the current
   runtime/native_ wildcard. Never link the reference adapter or guest acceleration
   into native. `NATIVE-SOURCES.json` lists the actual exercised source closure
   and additions relative to this packet base; deduplicate against current Main.
4. Register `sonic-controller-native` and `sonic-controller-reference`, both with
   no arguments, in tests/unit/manifest.tsv.
   Add the new fixture/Python/tool hashes; apply the reviewed legacy allowance move
   in `LEGACY-ALLOWANCE-MOVE.json`, and the Sonic
   readability reductions. No new game-tier allowances are needed: the current
   readability check passes with zero regressions. The proposed legacy map is an
   ignored verification artifact, not a global protection edit.
5. Review new observation/holdout helpers and the field lenses independently,
   then integrate into Main's current shared owners and run native closure,
   connected journeys and one coherent gate. No checkpoint movement, completed
   frame, N3b completion, physical controller or Windows/gameplay parity is claimed.

Reproduce from source with:

```sh
python3 tools/oracles/sonic_controller/build.py build/sonic-controller-check --assets /Users/burbujamc/Developer/sadx-macos/build/assets
python3 tests/python/test_sonic_controller_observe.py
```

The original verifier files remain byte-identical to ca640c5. Their provenance
source lists still name the old two Sonic files; Root should separately add the
new core/internal/site and reference adapter headers/sources to report metadata.
The full current compiler dependency fingerprints already cover them here.

The old/new callback witnesses prove publication-observer invalidation, live
parameter reloading and per-helper motion-owner capture. Native witnesses also
cover captured primary replacement, released links, canonical partial allocation,
actual collision records, both ordered forced reads and the neutral event seam.
Setup errors and earlier low-coverage mutation survivors remain recorded; the
corrected scoped-include witness replaces the invalid production forced include.

Inherited floor_angles/player_reaction failures remain open. Local-speed original
comparisons seed the transformed suffix; they do not prove the full producer.
Lower unchanged player modules retain their own readability/fidelity debt; the
bounded native harness is not whole-game shipping qualification.

Remaining native calls include editor/event bindings; model graph and original
common tail; hold/position collision composition; ground direction/yaw owners;
sound/effects/camera; idle/walk/special-jump/bounce/stage/damage operations. Existing
unrecovered action bodies stay explicit. This packet does not recover new physics,
event ownership or imports. No full gate, Main writes, push or reserved vectors.


## P1 physics capture correction (2026-11-02)

Before changing source, a fresh old/candidate witness proved same-record replacement:
baseline yaw modifies original speed 3 to 10; candidate modified replacement
103 to 110. Both read the new parameter 19. The native witness independently
proved mixed old state/new contact (acceleration X 4.225 instead of 0) and the
wrong early stop after null replacement. Both originals are retained explicitly.
The preserved BEFORE JSON files include the failed candidate observations.

`PlayerBorrow` carries separate non-owning record and captured physics pointers.
Every actual helper entry takes its own borrow; getters no longer reload physics
through the mutable record. Parameters retain their intentional live access;
movement retains its parameter reference and motion owner. Charge cleanup uses
its caller's capture. Floor inverse and finalize take fresh physics borrows rather
than inheriting copy's earlier borrow. Finalize still receives copy's captured
motion/gravity dependency, matching the old helper argument boundary.

Eight old/current reference and seven native scope cases pass. They cover same
record replacement and null, actual old/new field mutations, movement recapture,
movement's retained capture across its own callee, copy/finalize recapture, live
parameters, and contact reads after input. A nonreturning yaw observer records
real ground arithmetic; a separate normal-backend harness also passes. No physics
values, parameters, or new physics owners are copied/retained by the borrow.
The precise reviewed reload is reintroduced in a private reference copy and the
nonnull repro fails; null cases are intentionally not run against that UB-prone
mutant. All six prior faults are still caught. Both complete 58,992-record streams
remain exact, and the unchanged 3,072-case extra cohort preserves its 204/219 FP
partial identities. See `PHYSICS-CAPTURE-VERIFICATION.json` for final-source
fingerprints and all individual inherited discrepancies.

Reproduce the focused correction with:

```sh
python3 tools/oracles/sonic_controller/physics_capture.py build/sonic-capture-check --support build/sonic-controller-native/qualified --old build/sonic-controller-native/baseline/player/sonic_player.dylib
```

Root should additionally register `sonic-physics-capture-reference` (no arguments)
and the controlled `sonic-physics-capture-native` observation recipe. The latter
renames only the uncomposed yaw stub in its private native-backend TU and provides
a throwing observer; it must not be mistaken for a shipping yaw binding. No shared
schemas or Root composition files changed. A FRESH independent recheck is pending.
