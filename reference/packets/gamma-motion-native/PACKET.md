# Gamma native motion composition candidate

Base Main is `3b36f63b`. Supplied worktree is `t3-390f829d`, branch
`packet/gamma-motion-native`. Source commit is `ed00b1e3`. Independent review,
Main admission, shared-header reconciliation and the next full gate remain Root-owned.
No Main write, push, agent, fresh Ghidra session, particle change, reserved Task/reset
execution, inventory regeneration, schedule or provider operation occurred.

Gamma now calls the existing `animation::update` through the actual initialized
physics and player allocation. The imported immutable motion closure owns two
loaded action payloads; the existing 91-row table and 78 declarations retain their
identities, loaded/null/unavailable distinctions and exact reasons. No second
animation algorithm, cloned table, geometry or mutable pose owner was introduced.
The same validated MDATA2 adapter now serves Sonic and Gamma. CharacterPack still
rejects unsupported layouts centrally; no MDATA3 pointer bits or adjacent backing
are fabricated.

`animation::Runtime::scene_objects` borrows Session's `character_scene_.objects`.
The shared engine delegates object lookup, hierarchy links and pose reads to its
access binding. Native access points directly into ObjectState; the old optional
SceneGeometryObject map remains solely a compatibility input for existing
reference/native fixture probes. No native caller fills or caches that map.
The reference adapter retains the same lazy reads and failure phases. Sampling
still uses the unchanged maintained engine, exact arithmetic, arena and key owner.

## Actual caller and next boundary

With the same retained save inputs and normal key events, the real source-built
app stops at `native_gamma_join_vertices`, frame 1332, exit 1. Its inspected capture
is byte-identical to Main's outgoing title image:
`c7d02f642ae8e5493f19051e7cf13a06a2d1d103351e72b0576a108b52aba427`.
A separately recorded empty-save run reached that same boundary at frame 1082;
that was a different save input, not a frame-comparison result.

The actual first PSetMotion invocation returns after allocating its action,
motion and zeroed 111 by 24-byte node buffer. Request/current/previous are zero,
mode becomes Select, frame/start stay zero, timer becomes 1 and flags zero.
The generated motion has two frames, type 3 and input layout 2. It is too early
to claim an animated/rendered actor frame. Gamma entry publishes its previously
captured null player alias before PInitialize; that current-player slot remains
null on this call. PSetMotion borrows its passed real player instead. The primary
alias still identifies the real task. No alias is repaired or manufactured.

The next caller contract is PJoinVertexes, with the same Primary, motion handle
and initialized Player passed by `common_tail`. The player's join table is the
existing five-row Gamma weld table, whose object pointers borrow the same
SceneState objects and geometry. Its animation tail already owns the generated
motion allocations and the unchanged canonical table. PJoinVertexes must preserve
these owners and the entry-time current-player alias. Laser targets, model matrices,
fan, display, effects, collision and children still follow it in the original order.
No part of that tail was bypassed or newly implemented by this packet.

## Checks and limits

Eight focused programs pass: the new complete Gamma caller; unchanged real-DLL
animation lifecycle under 12 FP profiles; 114 interface characterizations; 145
export reads / 882 publications and 150 native prefixes; two-player Gamma shared
binding; accepted Sonic assets/two real players/Stand; actual Sonic callback
integration; and the unchanged Gamma effects caller. The older effects witness
omits the new owner intentionally and retains its old explicit PSetMotion stop.
The new witness supplies the real owner and reaches PJoinVertexes.

The new witness also checks initial allocation contents, real live speed/charge
references, stable identities, resource lifetime, ordered unavailable-row stores,
77-node Stand keys and real Stand-to-preview transitions after two live pose edits.
The controlled multiplier checks attach a copied test table temporarily to select
speed/charge modes; the production table remains immutable and is restored.
Optimized and UBSan witnesses pass. Six compiled mutations and two observer
negatives are caught; `FAULTS.json` records their exact assertions.

Original instruction comparisons retain their limits:

- Motion: 16,440 cases, zero value/boundary changes, all 4,140 individual FP partial
  identities preserved against locally source-built before/current libraries.
- Blend: 7,944 cases, zero value changes, all 1,668 individual partial identities
  preserved. No fixture, input, tolerance or old verifier changed.
- Sonic table: unchanged 103 cases pass; the imported table comparison retains
  all 2,352 bytes. Sonic's accepted two-player/Stand and publication witnesses pass.
- Gamma core: 1,422 cases, zero value failures, 498 FP partials, strict exit 2.
  Its summary supports counts only; no individual Gamma partial identity claim.
- New real imported Gamma transition comparison: 465 rows across three scenarios,
  zero stored/key/state differences. Stand is exact; the two preview transitions
  have source precision flags 0 versus native 32. Strict exit is 2, not pass.

App and force-load closure build from maintained source with `-j2`. Only local,
dereferenced SDL3/libusb products were copied; no writable dependency destination
points into Main. Readability has zero regressions; layer/provenance/global-owner
checks pass. Eleven conservative source coverage windows have zero active
unhandled or undetermined profile overlaps, including the inactive NodeLimit patches.
No fresh source archive without original images, full gate, hardware comparison,
completed actor frame or N3b claim is made.

The unchanged judge check intentionally remains unsealed: 53 inherited ignored
binary inputs are absent in this worktree; two registration files changed and four
new protected files need admission. All 1,389 inherited manifest entries remain
byte-for-byte unchanged. `JUDGE-PROPOSAL.json` supplies exactly those six finite
updates and preserves every inherited path. Root must reconcile the current
inventory and seal the registration after integration; do not broadly regenerate
or drop absent binary protections.

## Evidence and reproduction

`verify.sh` runs the bounded native/asset checks and six compiled faults.
`verify_imported.py` executes the pinned original PSetMotion and transition bodies
with real original Gamma tables/actions and controlled allocator suppliers. It
compares every generated key word. It does not compare the original allocator policy.
`verify_preservation.py` imports the old verifiers unchanged and retains complete
before/current observations plus every individual partial identity. Its before
libraries use the six source/header snapshots from `git show 3b36f63b`, linked to
the unchanged local native support members; exact paths/hashes are in VERIFICATION.
No game/model/key bytes, screenshots or executable artifacts are committed.

All raw reports, native libraries, source snapshots, observations, mutation products,
logs and inspected images remain under this durable worktree's ignored
`build/gamma-motion/`, not under temporary storage. `PRESERVATION.json` pins their
full-report digest and individual identity-list digests. VERIFICATION records the
actual commands, statuses, attempts and current source/test fingerprints.
