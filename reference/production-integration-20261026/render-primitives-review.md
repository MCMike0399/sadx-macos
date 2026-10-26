# Independent render primitive review — 2026-10-26

Reviewer branch: `packet/review-render-primitives`. This review covers the two frozen
components below, independently of Root's runtime integration. **Both receive bounded
component approval; neither receives a strict FP pass, runtime approval or gate certification.**
No blocking fidelity or manual STYLE §12 finding was identified in either component.

## Scope and method

* A: `1960fed6b8a982138cce6c54b3bd7a53d3f25f6b`,
  `packet/normal-uv-strip-core`, in `/Users/burbujamc/.t3/worktrees/sadx-macos/t3-791addd8`.
* B: `1a5a0f3a7b09a7a9390f421d158b11255ffa958d`,
  `packet/material-correction`, in `/Users/burbujamc/Developer/sadx-wt-material-correction`.
* Both diffs were examined against `04c40ae0b0840b9982cda95452057c1d678eeec1`.
  The pre-existing Root gate does not include these packets.
* Read AGENTS/CLAUDE instructions, GOAL, STYLE, current STATUS and last HANDOFF,
  relevant canonical normal/UV and graphics-device memory, both PACKET files,
  engine files, units, probes, new oracles, checker/mutants and proof/profile records.
  The code-review skill's standards/spec split was applied in this reviewer only;
  the task's explicit prohibition on workers overrides its parallel-worker procedure.
* Source checkouts were read through frozen `git show`/diff operations. Retained raw
  dossiers and A's retained full reports were read directly as evidence. No source
  checkout was switched, renamed, edited, compiled or executed.
* Read-only Python checks used `/opt/homebrew/bin/python3` in a login shell. They
  verified the entire EXE SHA-256
  `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`,
  all declared A/B source-range hashes, all nine A dossier hashes, B's dossier hash,
  A's seven frozen packet-file hashes and B's four file hashes in each report.
  A's retained dev/holdout report hashes match the frozen proof. The original scale
  at `0x7db2c4` is exactly `81 80 80 3b`.
* No unit, oracle, mutant, checker, gate, application or Ghidra run was performed by
  this reviewer. Test results below are inspected producer receipts, not reruns.

## A — normal/UV strip and PolyBuff helpers

**Verdict: approve this recovered component with its stated FP and integration limits.**
Standards: zero blocking findings. Spec/fidelity: zero blocking findings.

The source evidence is `normal-uv-strip-source.json` and its nine hash-checked
dossiers. For example, strip `ev_3d7b1932…`, Lock `ev_4a6769d9…`, Draw
`ev_d6761dbc…`, Init `ev_b5a58dd3…` and initialization context `ev_da94eca7…`.
The eight implemented entries are distinct from the ninth, unimplemented
`InitPolyBuffers` entry. I compared each implemented body's retained assembly to
the engine algorithm and receipt wiring.

Objective fidelity checks and locations, using frozen source line numbers:

* `src/engine/normal_uv_strip.cpp:17`: signed16 indices select three raw component
  words each from points and normals; copies precede color, then signed U and V
  load/multiply/binary32 stores. The literal scale matches source m32 bytes.
  No normalization or replacement strip algorithm is hidden in a probe seam.
* `normal_uv_strip.cpp:139`: low 14-bit length mask, prepass count, entry color/cull
  snapshots, retained index/UV identities, post-Lock strip-count read, initial
  duplicate and trailing duplicate agree with the strip assembly. Zero length
  still reads the initial index, then the preceding header index/UV pair.
  The API uses `(mesh, points, normals)` rather than adopting the old reversed
  header declaration; that conflict is expressly documented in PACKET.
* `normal_uv_strip.cpp:88`: unsigned wrapping products/subtractions, strict
  `bytes < remaining`, equality DISCARD, remaining store before division,
  primitive-count/start/cull stores, increment before Lock and stale stride-valued
  output agree with `794630`. Failed HRESULT is not a success guard. Unlock's
  actual result is returned at line 110 and ignored by strip/discard callers.
* `normal_uv_strip.hpp:51`: row multiplication wraps in bytes before selecting
  argument words. `normal_uv_strip.cpp:114` captures count and argument allocation
  identity, reads cull before SetRenderState, then reads count/start after it,
  and clears lock count only at the end. These match Draw's register lifetimes.
* `normal_uv_strip.cpp:39`, `:68`, `:128`: Init clears only start/count in each
  allocated row, preserves cull/fourth word, and has no invented allocation guard.
  Free clears only resource/argument identities. Discard reserves one stride,
  issues DISCARD Lock then Unlock, and leaves the argument rows and lock count.
* `tests/probes/native_normal_uv_strip_probe.cpp:1` and the new original oracle
  keep algorithms in the engine and original instructions, respectively. COM and
  allocation receipts supply platform facts, not triangle construction. Bounds
  checks stop at consumed unavailable data; the source judge retains the actual
  instruction/address. State, ordered API/store prefixes, final successful vertex
  words, owned argument words and returns are compared. Low-six FP flags are
  compared separately and explicitly excluded from the receipt hash, not ignored.

Inspected results: 12 native checks; development 1,014 and holdout 2,662 cases with
zero value/prefix mismatches; 15,720/29,088 ordered receipts and 9,309/14,934 vertex
word stores. Seven compiled semantic mutants are recorded killed, with nonzero
value mismatches: equality, mask, signed index, scale, initial duplicate, trailing
UV and argument order. **Both oracle exits are 2, FP partial**, with 154/198 flag
mismatch rows (native inexact versus source clear). Both `passed` fields are false.
The holdout has a different seed and larger random sample; it shares the generator
and deterministic edge cases with development, so it is not an independent harness.

STYLE §12: named functions and seven symbol overrides agree with the pinned
direct3d declarations; original entry annotations are confined to the header.
Eight recovered functions are short and structured, with NOTSA helpers rather
than an address dispatcher. The borrowed Context, device resource and CPU bytes
do not form a service hierarchy. ExactFloat expressions retain caller precision
without emulator operations in engine code; no entry census justifies weakening
that contract. Constants are maintained literals; input content is borrowed.
Platform seams are tagged `[platform:d3d8]`; no unrecovered algorithm is a silent
no-op. Observation seams observe engine-decided stores and must remain observers.
No enhancement was added; future hooks belong at named calls under STYLE §9.
The recorded ten-category new-file readability debt is zero; no baseline was raised.
Profile receipts recognize all eight complete implemented ranges with zero
UNHANDLED overlaps. Judge-manifest changes register additions, preserve old hashes
except the permitted appended Make/unit registrations, and do not loosen a judge.

Informational limits/prerequisites, not component defects:

* `normal_uv_strip.hpp:91` and PACKET's integration contract: the probe has one
  resource identity and synthetic platform receipts. It does not prove real
  device ownership changes, aliasing between imported data and writable output,
  reentrant callbacks, actual driver failures or GPU snapshots. Root must bind
  real storage and per-call device state; a non-null resource cannot stand for CPU
  bytes. The local judge's pointer normalization is not runtime alias evidence.
* `normal_uv_strip.cpp:39`: Root must implement the actual `78e720` composition,
  preserve one canonical record 8 owner, pass actual configured capacity and bind
  stride 36/FVF 0x152. Neither Question's vertex count, PE 12000 nor default 32000
  alone is a receipt for the runtime's configured count.
* Full FPSW/tag/stack state and unmasked/deferred exception prefixes are unverified.
  The mutants meaningfully challenge strip and lock semantics, but do not establish
  all lifecycle, callback-alias and arithmetic-error behaviors by themselves.

## B — CorrectMaterial

**Verdict: approve this recovered component with its stated FP and integration limits.**
Standards: zero blocking findings. Spec/fidelity: zero blocking findings.

The source is full `40a340..40a3f4`, evidence
`ev_93b5607dad9b91f61c967d313222d9ddb617919575a33ea9d2ff782a063f3d89`.
Its dossier, body hash and scale bytes were verified independently of the prose.

Objective fidelity checks and locations, using frozen source line numbers:

* `src/engine/material_correction.cpp:14`: the GetMaterial result is a local
  17-word material; ambient red loads and multiplies before the Power read, Power
  is assigned before the first binary32 store, then remaining ambient, diffuse
  and specular RGB are processed in source order. Nine m32 scale multiplies retain
  the exact context; alpha and emissive are untouched. There is one final Set.
  The original's changing ESP offsets were resolved to their actual channel
  positions rather than transcribed as apparent offsets into one fixed layout.
* `tests/unit/material_correction.cpp:1`: the ExactObserver ordering assertion
  checks Get/Load/Multiply/Power/Store, eight more triples, then Set; a Power seam
  mutation of device emissive is overwritten by the original local-copy result.
  This supports operation order and publication isolation, not fault delivery.
* `tests/probes/native_material_correction_probe.cpp:1` and
  `tools/oracles/verify_original_material_correction.py:1`: all 17 input words
  are concrete receipts; both sides stop on unavailable Get/Set receipts. A
  Get receipt changes Power to a distinct value, making premature capture
  detectable. Original arithmetic is executed, not replaced with expected RGBs.
  Negative HRESULT receipts still write 17 words: this tests ignored HRESULTs,
  not the initialization behavior of a real failing driver.

Inspected results: development 4,227 and holdout 6,227 cases, zero value/API
mismatches, with **3,231/4,567 FP-status mismatches: partial, not pass**. Six masked
precision/rounding controls cover independently distinguished channels, special
values, NaN payloads, random bits and missing receipts. The CLI returns nonzero
for a partial (its implementation returns 1), and both `passed` fields are false.
Five value mutants compile and fail the value verdict; the unchanged runner's
log includes counts and restored partial baseline. Individual mutant JSONs are
not retained, so their complete failing-case details cannot be independently
audited from the committed log. The holdout's seed differs, but the generator and
fixed cases are shared with development.

STYLE §12: CorrectMaterial's name matches the pinned header. One short plain
function plus a NOTSA RGB helper uses semantic native fields, structured loops,
an annotated source entry and a verified literal; no addresses outside annotations,
guest constants, fixture optional fields or service hierarchy enter the game code.
Get/Set are explicit tagged D3D platform seams. ExactFloat has a borrowed live
context and explicit stores; simplifying it would need census/oracle evidence.
No invented Power owner/default or new enhancement is introduced. The recorded
new-file debt is zero across all ten categories. The profile report recognizes the
one exact full range and reports zero UNHANDLED overlaps. Existing Make content
is preserved with two appended targets; units gain one registration, and no old
expected values, tolerances or harness algorithms change.

Informational limits/prerequisites, not component defects:

* `material_correction.hpp:13`: Root must establish the real specular-Power
  producer, lifetime and single owner, then borrow it. BSS zero is not evidence
  for the value reached during rendering. Get/Set must bind the appropriate
  device state and preserve the caller's actual callback identity.
* `verify_original_material_correction.py:87`: six controls are all masked;
  unmasked arithmetic exceptions and partial material publication on such faults
  are unverified. FP-status disagreement cannot be converted into strict parity.
* The frozen B diff does not update `reference/judge-manifest.json`. Root must
  register its new protected inputs and appended Make/unit rows through the
  documented additions-only procedure before the combined gate. That is pending
  gate preparation, not evidence of changed existing expectations.

## Boundary of this approval

The source/probe correlation risk is reduced by executing original bodies and
checking original bytes, but synthetic API receipts cannot judge their future
native bindings. Source review supports these bounded algorithms only. Root still
needs real ownership/binding checks with distinct nonzero values, source-only
product linkage, composed drawn-content checks and the combined full gate.
Neither packet establishes GPU parity, completed model rendering, N3b, gameplay,
63 FPS, or full floating-point exception fidelity. No tests were rerun, no memory
files changed, no workers launched, and no source/judge/fixture edits were made.
This durable review records the useful conclusions without duplicating packet
memory or writing outside the assigned review worktree.
