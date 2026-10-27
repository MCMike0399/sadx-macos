# Gamma weld assignment and cursor initialization

Branch `packet/gamma-weld-cursors`, based on Main `bfceb0a`.
Status: qualified private implementation, source commit `110db08`. Root reported
its combined gate terminal rc 2 (idle windowed smoke) and explicitly released
private qualification before any build or execution here. The source-only draft
commits remain preserved. Root owns Main integration, registrations, fresh
independent review, source-only export and the full gate. No provider ran here.

## Scope and ownership

The next actual E102 store assigns the imported five-row GammaWeldInfo table to
its captured player's join table. PlayerAnimationTail owns this field and the
initialized native view borrows it. PlayerSystem owns the one mutable global
Gamma weld table. Its object pointers borrow the existing SceneState ObjectState;
there is no per-player table copy or extra scene. Original `org` retains its real
allocation identity (four point/normal triples, 48 bytes per vertex); this slice
never allocates it or implements PJoinVertexes. The pair pointer is unsigned16.

The SDK WIREMODEL input/output fields are typed raw-word/resource views and arena
allocations. Existing allocation-keyed GammaExtraWork owns cursor1 and cursor2.
The actual caller pushes extra work + d0, then + b0, before calling the two-pointer
initializer. Its first destination is cursor1 (+ b0), second cursor2 (+ d0).
No new session attachment, service hierarchy or current/player/motion owner exists.

GammaActorAssets holds imported declarations. Session's existing load_player_assets
binds them to its SAME CharacterPack/SceneState. GeometryState gains canonical color
word arrays and leaves vector/index values and identities unchanged. Template globals
are distinct live owners, not expanded fake-success buffers. The initializer reads
and copies template1, expands it, then reads/copies template2 and expands it.

The expander captures signed16 count and input views before either allocation,
allocates count * 24 point bytes and publishes that result, allocates count * 8
color bytes and publishes it, clears texture and stores doubled signed count.
Each signed16 indexed vertex copies three raw point words followed by one color
word, with each write observable separately. Failed allocations retain the source
prefix; overwritten allocations remain live. Interior native word views allow
negative signed indices into valid preceding backing storage without pointer casts.

## Complete source evidence

Root's closed owned dossiers under
`/Users/burbujamc/Developer/sadx-wt-normal-uv-strip/reference/inventory-raw/`:

* `gamma-weld-cursor-recovery-20261026/gamma-next-43fa90.json`:
  ev_ebda3d9ab7749eb17fae5a4747cf23de66cd4600597882eb53209b2b361adecb.
  Full consumer context only, including org allocation and unsigned pair reads.
* `gamma-next-4c2e10.json`:
  ev_6d92e26672ef1c99538c60fd2466b1c32801a4975c46cb4ead2b59254a046022.
* `gamma-next-4c2d40.json`:
  ev_4b46549539a0c717d0b670a25e4318f71355e35521602abb7bc5bb67739f3485.
* `gamma-selection-recovery-20261026/gamma-483430.json`:
  ev_2f86a86b5e458c4a46ed9ceba4c3356e895691686cb9a4cd3c3e96ec964bf27f.
  Exact weld store and both caller destination LEAs.
* Complete producer `gamma-7cb560.json`, SDK SADXStructsNew.h PL_JOIN_VERTEX,
  WIREMODEL and E102WK. Existing approved Gamma asset packet 0b5113d documents
  all five joins and the two templates with shared ResourceIds/inverse proofs.

## Qualification

Cursor original CPU cluster: 768 development cases, zero mismatches, 1,920
allocator receipts and 444 failure-prefix cases. Eight meaningful compiled mutants
are caught by the development judge. Final untouched 768-case holdout has zero
mismatches, 1,962 receipts and 435 failure-prefix cases. Both verdicts are strict
pass; the source routines do no FP arithmetic. MAlloc is an ordered receipt,
not an independent re-proof of the existing allocator.

Portable cursor unit and actual scheduler/NativeAccess/pack/arena composition
unit exit 0. They prove signed preceding-word access, int64 invalid-view bounds,
canonical identities, imported inverse copies, real allocations and failure
prefixes, leak retention, captured inputs and delayed live template2 reads.
Unchanged Gamma and startup units and the unchanged vector/index geometry unit
exit 0. Private product app and force-load native closure build/check exit 0.

Unchanged Gamma original development/holdout retain 1,422/1,080 cases, zero value
failures and 498/690 FP flag differences, actual script rc 2: both remain partial.
Unchanged startup retains 2,004 cases, zero value/boundary/resource/control/fact
mismatches and 924 FP flag differences. Its script returns 0; verdict is partial.
Original core/probes/judges/mutants and CPU baseline are byte-identical to bfceb0a.

Readability, layers and one-owner checks exit 0. Every new game file has zero
unlisted debt. Provenance exits 0 using an explicitly constructed candidate symbol
database containing the three proposed entries, not an edited canonical symbol
source. Root must register those entries. Full two-range profile coverage has
zero unhandled and zero undetermined overlaps. No baseline change was made.

## Actual native route

The exact Root normal-input route, with a fresh private save directory and shared
assets read only, exits 1 at frame 1382 at `native_gamma_load_setup_resource`.
Gamma preview is source mode 1/ID 2/player slot 5; it has crossed the actual weld
assignment and both cursor expansions. This does not certify mode 1 or its later
common tail. Loaded Gamma texture count remains 2 and texture slots remain 762.
The inspected screenshot still shows the outgoing title screen, not actor pixels.
Full command, application/asset/source/log/image hashes and all limits are in
`proof.json`. Raw logs/captures remain private under `build/gamma-weld-cursors`.

No PJoinVertexes, actor drawing/GPU, N3b, physics, story or gameplay claim. Later
calls and the full common tail remain fail-loud seams. No full gate, provider,
Ghidra, subagent, Main change or push was performed by this worker. Root's rc 2
gate has not been presented as green. Existing consumer judge updates and all
new registrations remain Root owned; see `registration-proposal.md`.
