# Shared current player owners

Branch: `packet/player-current-owners`. Base: `c29d687`.
This packet needs a fresh Root-initiated STYLE §12 review before Gamma/native consumption.
It does not qualify N3b, rendering, gameplay, the complete Sonic controller, or native closure
of that controller. No app, full gate, Ghidra provider, push, or shared checkout write was run.

## Scope and contract

`src/player/actor_current.hpp` defines `sadx::actor_current::State` with plain `primary`,
`player`, and `super_sonic` values. `SonicPlayerGlobals::current` owns exactly that State type
once. A native Session can own the same type independently of the legacy controller; it must
not create a second current state for Gamma. Gamma's `CurrentView` should borrow
`{current.primary, current.player}` as mutable `TaskWorkHandle&` references. Session/Gamma
binding and native source registration belong to Root and are not edited here.

The source owners are `gpCharTwp` at `0x3abdf60` (4 bytes), `gpCharPwp` at `0x3abdf64`
(4 bytes), and `SuperSonicFlag` at `0x3c55d45` (1 byte). Each has one GLOBAL annotation.
The first two names come from `SADXVariablesNew.h`; the third from `SADXVariables.h` and
the current symbol database. This byte is distinct from `flgPlayingSuperSonic` at
`0x3b18db4` and `gu8flgPlayingMetalSonic` at `0x3b18db5`; neither is migrated here.
All nine bytes are in the pinned PE's zero-filled virtual `.data` tail, beyond raw data.
The old three value fields and six spelling allowances are removed. Historical source-extent
checks stay intact and are satisfied by the annotated owners. The host arm64 State is 40
bytes (16-byte handles at offsets 0 and 16, byte at offset 32); it is not a guest layout.

Only source publications change the slots. Initialization or registry changes must not
refresh `player`: initial NULL or an older player work remains until the next source store.
`set_primary`, `set_player`, and `set_super_sonic` store first, then invoke an optional flat
Observation callback. Missing records/empty callbacks cannot veto stores. Same-value/null
stores are observable; a source `1` then `0` sequence must remain two publications. Callback
exceptions propagate after the store. Readers return the authoritative handle by const
reference or the exact byte without boolean normalization. State fields support direct mutable
borrowing. There is no registry, manager, virtual observer, emulator dependency, or new Calls
interface. Sonic algorithms remain in their original file; only three stores changed.

## Frozen judges and results

`reference/production-integration-20261026/player-current-owners/proof.json` freezes all 13
existing probe/unit/verifier/fixture hashes, live baseline and final verdicts/case digests,
source-zero evidence, compiler/link inputs, and seven caught mutants. Every frozen file is
byte unchanged. The three frozen units are Sonic player, Sonic actions, and scene-position
binding. The last retains 136 actual ECoast paths and 96 retained responses.

Both baseline and final Player runs have 28,800 cases, zero stored mismatches and 2,840 FP
status differences (ground-prefix subset: 4,032 cases, zero values, 1,632 FP differences).
Both Actions runs have 30,192 cases, zero stored/boundary mismatches and 2,172 FP differences.
Case digests are unchanged. Both verdicts remain **partial**, not pass. The existing
verifiers return rc 0 for these partial reports; `passed` remains false.

The new registered core-only unit checks the identical legacy State type, zero defaults,
mutable borrowing and identity, stale/null player publications, observation order, same-value
stores, empty callbacks, a throwing observer after publication, and all 256 byte values plus
wraparound across 257 independent holdout worlds. It passes. Seven compiled mutants are
caught: dropped primary store, wrong player slot, suppressed null publication, observation
before store, suppressed same-value publication, store conditional on an observer, and byte
normalization. The unit also links from only actor_current.cpp + task_work.cpp and System/libc++.

Provenance, owner, layer, readability, profile and whitespace checks pass. New source debt
is zero. Sonic address-identifier debt decreases cpp 174→171 and hpp 9→6; all other metrics
stay fixed. Profile check covers the unchanged Player report's five original code ranges,
with zero UNHANDLED overlaps; the legacy Actions report declares no addressed ranges.
No new original code range was recovered in this ownership refactor.

## Compiler recipe and Root setup proposal

All builds were private and at most `-j2`, using a login shell and `/opt/homebrew/bin/python3`
for tooling. Fresh core and reference archives were compiled from this worktree. The source
EXE, constants and case corpus were read from the existing root checkout without modification.
Large local raw evidence and executable products remain ignored under
`build/player-current-proof/`; no assets or binaries are committed.

Build the core with `make -j2 build/libsadx-core.a`. The reference archive recipe used
`make -j2 build/libsadx-guest.a UNICORN_DIR=/Users/burbujamc/Developer/sadx-macos/reference/python/lib/python3.14/site-packages/unicorn`.
Compiler flags: `-std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math
-arch arm64 -mmacosx-version-min=14.0 -Isrc -Itests/fixtures`.

The byte-frozen fixture TUs still use old optional syntax. Compile only the two Sonic
probes and three legacy unit TUs with
`-include tests/reference/actor_current_fixture.hpp`. The facade derives a fixture-only
Globals type whose three receipts hold const references and availability booleans, never
another source value. Its observer is attached before fixture execution. It is noncopyable
because callbacks and references borrow that exact instance. Never inject it into a game TU.
Link legacy probes/units with reference archive then core archive, as before. The old
`get_player_acceleration_ground` upward linkage remains reference-only; do not link guest
to qualify the native product. New owner-only code has no such dependency.

Run the unchanged `verify_original_sonic_player.py` / `verify_original_sonic_actions.py`
with the original EXE and new dylibs, `PYTHONPATH=tools/lib:tools/oracles`, and the existing
reference Python (Unicorn 2.1.4). `verdict.py` must still report both partial counts above.
Run the three frozen units with their unchanged manifest arguments. For the new standalone
unit, compile `tests/unit/actor_current.cpp src/player/actor_current.cpp
src/engine/task_work.cpp` with the flags above; it requires no assets or guest archive.

Root owns the protected Makefile fixture flags, judge-manifest additions, native-sources
registration of actor_current.cpp, Session binding, fresh independent review and full gate.
Those setup edits were not made here. Root should not consume only the new header: include
the Sonic field/store migration, shrinking allowances, reference facade and proof together.

One bounded static salvage used tools/agents/extract_rollout_files.py on the 2026-10-21
17:31 rollout, selecting /private/tmp/sadx-native-sonic-current-stage. It yielded zero files,
71 unsupported operations, and an incomplete verdict. No recovered command was executed;
the implementation and checks above were performed afresh against current source/judges.
