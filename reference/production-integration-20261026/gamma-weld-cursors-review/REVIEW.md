# Independent Gamma weld/cursor review

**Verdict: bounded approval; no source fidelity or STYLE blocker found. Tests NOT RERUN.**

Reviewed frozen `19e20c88f1cb18f2412cd8c80ac20e6e822980e5`, source
`110db08733b161e9318a0bb005b882ffb530cb82`, against base
`bfceb0a9d3850eebd75825b684b6f3de84cc2259`. Qualification documentation is
`864ce9a` / `19e20c8`. This fresh first review read AGENTS, GOAL, STYLE including
§12, STATUS top, HANDOFF last and JUDGE. Review branch:
`packet/review-gamma-weld-cursors`, starting at Root documentation `cf1c51a`.

Only this review directory was written. No build, test, oracle, app, checker,
Ghidra, provider, subagent, Main/shared-worktree mutation or push was performed.
Read-only inspection included saved reports, logs, source dossiers and the saved
frontier image. Hash comparisons and their results are in [audit.json](audit.json).

## Source and spec

* The real E102 caller stores GammaWeldInfo at player +144 after ordinary
  LoadCharTextures (48362a). The implementation borrows one five-row mutable
  PlayerSystem table through the actual PlayerAnimationTail gap. NativeAccess's
  initialized view borrows that field; calloc establishes known null. No new
  current/player owner, PInitialize replacement or Super store was introduced.
* PL_JOIN_VERTEX matches the SDK's three object pointers, byte count/mode/depths,
  org allocation and unsigned16 pair indices. Complete consumer 43fa90 confirms
  allocation of 48 bytes per vertex and four point/normal triples. This is type
  evidence only: original_vertices stays null, and later PJoinVertexes still
  throws `native_gamma_join_vertices`. There is no invented weld computation or
  static pose.
* The importer validates all five declarations, including the sentinel and
  source-null fields. Binding resolves objects through the existing ObjectState.
  Templates borrow the existing GeometryState vectors/indices and its added
  color-word storage. Colors are decoded once per resource in that canonical
  owner; there is no per-Gamma color cache. Existing vector/index decoding is
  unchanged. Session's existing load_player_assets calls a plain resource binder;
  there is no attachment, vtable or service expansion.
* Caller assembly 483650/+d0 then 483657/+b0 pushes the second then first
  destination. Allocation-keyed GammaExtraWork cursor1/cursor2 follows that order.
  Complete 4c2e10 copies all eight words to the first destination, expands it,
  then reads/copies template two. Native assignment retains all semantic fields,
  including padding and output fields, with that delayed second read.
* Complete 4c2d40 captures MOVSX line count and all three input pointers before
  either allocator. Native code does likewise, with count*24 then count*8 bytes,
  separate result publications, null texture, doubled signed count, signed16
  index, and three ordered raw point-word stores followed by indexed color.
  No FP arithmetic is introduced. Unsigned allocation-size conversion preserves
  x86 wrap for negative counts; the loop stays signed and skips nonpositive counts.
* WordView retains a borrowed vector and origin, so signed negative indices can
  reach valid preceding words. The int64 host position prevents overflow in the
  bounds calculation. Imported templates use origin zero. Reads remain live;
  input descriptor capture does not copy array values. Per-word failures retain
  earlier stores; overwritten arena blocks remain live. Named errors distinguish
  read/write boundary, vertex, component and allocator-work failure.

## STYLE §12

1. New game files use structured semantic code, with addresses confined to
   provenance annotations. No guest dispatch, fixture optional or CPU emulation
   was added to the cursor implementation. Saved readability output reports zero
   regressions; CPU/readability baseline is byte-identical to base.
2. Existing SDK names are retained. Three descriptive symbol proposals are
   explicitly pending canonical registration, rather than represented as applied.
3. Source side-effect ordering and its unusual leak/publication behavior are
   preserved and commented, as detailed above.
4. Arithmetic is plain width-explicit integer/raw-word copying; FP remains outside
   this slice. Existing Gamma/startup arithmetic is unchanged.
5. Resource content comes from the imported pack. No runtime EXE parsing or
   invented constant source is added.
6. Setup, material/effect/motion and PJoinVertexes boundaries remain loud. The
   reviewed caller reaches setup before those later behaviors can be claimed.
7. No existing protected path changed relative to base. Original Gamma/startup
   judges/probes and named frozen inputs match their receipt hashes and base
   blobs. New cursor judges and mutants are additions awaiting Root registration.
8. Imported resource binding and the existing initializer seams give later callers
   and content work a clear boundary; no speculative enhancement framework is
   needed for this exact copy algorithm.
9. Added bounded-view/resource-binding helpers carry NOTSA explanations. Source
   fields trace to SDK, complete consumer, caller, producers and template data.
   The one-owner arrangement does not move game logic into a reference adapter.
10. This adds no new Win32/D3D/DirectSound replacement site. Allocation uses the
    existing native arena; there is no new platform implementation to annotate.

## Evidence audited, without execution

All 20 source/test/tool hashes in proof.json match frozen Git blobs and packet
disk files. All ten frozen-input hashes match base. No existing judge-manifest
path or the manifest itself changed. All three closed dossier IDs/hashes match;
the complete expander and initializer bodies contain 164 and 56 bytes respectively.
Profile evidence covers those two full ranges with zero unhandled or undetermined
overlaps. The reported 79 globally active unhandled patches are outside this
bounded range result; this is not a global profile-completeness claim.

Saved development and holdout each report 768 cases, strict zero mismatches,
444/435 fault-prefix cases and 1,920/1,962 allocator receipts. Retained raw reports
match committed reports. MAlloc is a receipt model using arena rounding and
exhaustion, not an independently proven allocator. The oracle compares semantic
descriptors, payloads, fault prefixes and publication observations; receipt totals
alone do not establish every request. Large positive counts are bounded by
exhaustion before unavailable array elements. These limits are disclosed.

All eight mutants have recorded compile rc 0, oracle rc 1 and positive raw
comparison mismatches (214, 311, 303, 104, 150, 232, 192, 126). I inspected their
single source edits and retained mismatch examples: signedness, stride, captured
input, indexed color, early/substituted template and late publication defects are
detected through values or expected semantic fault-prefix differences. Compile
errors and harness crashes are not credited. No independent execution was done.

The new portable and composed units cover arena pressure, leak retention, negative
preceding indices, invalid host origin, per-component prefix, delayed template
mutation, canonical join/object/geometry identities and actual callback/current
ordering. The saved receipts report those and unchanged Gamma/startup/geometry
units, product build and force-load closure rc 0. Original Gamma remains partial:
1,422/1,080 cases, zero value failures, 498/690 FP differences, script rc 2.
Startup remains partial: 2,004 cases, zero value/boundary/resource/control producer
mismatches, 924 FP differences despite script rc 0.

Saved executable/log/image hashes match proof.json. The saved normal-input log
ends at native_gamma_load_setup_resource, diagnostic rc 1/frame 1382, with Gamma
mode 1/ID 2, loaded count 2, 762 slots and current player null. The inspected saved
image shows the outgoing title, matching c7d02f642ae8e549…; no actor pixels are
qualified. An assigned callback is not proof its display path completed.

## Root integration obligations and limits

These are explicit proposal omissions, not native algorithm findings:

* Add gamma_cursors.cpp to native-sources.txt and the three symbol proposals to
  the canonical symbol source.
* Register both units, isolated probe/oracle recipes and new protected paths under
  JUDGE's applicable additions/separate-commit rules.
* Update the old native_gamma_binding consumer's preceding weld-stop expectation
  separately with evidence; preserve original core judges and expectations.
* Rebuild the integrated/source-only product and qualify the combined full gate.

Root's latest combined full gate actually exited 2 at the sleeping-display
windowed smoke. The 275 native/400 Python passes and separate R1–R4, three native
journeys and 16 startup checks do not turn it green. Last successful full gate
remains the earlier Gamma integration gate at the CCL stop. GUI restoration and
a new complete gate remain pending. No approval here certifies actor rendering,
GPU parity, N3b, later Gamma mode/common-tail behavior, story, gameplay timing,
physical controller delivery or a source-only integrated build of this packet.
