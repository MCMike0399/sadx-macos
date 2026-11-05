# Stage direction normalized; presentation stops in Gamma target effects

Branch `packet/draw-path-4`, base `6491596`, implementation `6acd31d`.
The palette-0 cleanup now completes. Normal input still stops before presentation:
source `0x484102` calls Gamma Display `0x47fd50`, then source `0x48410d`
(`e8 8e 0c 04 00`) calls `GammaUpdateTargetEffects` `0x4c4da0`.
The explicit native seam reports `native_gamma_update_target_effects` because its
behavior remains unrecovered. No store, actor, cleanup or continuation was skipped.

## Inspected frame and unchanged drawn-goal checker

The final offscreen Metal run requested frame **1550**, using the private asset
tree and fresh copies of all three hash-verified saves. It exits **1 at frame
1332**, with **1330 completed frames**, native game mode 12/state 21. Both source
save hashes and their private initial copies are verified; source saves are unchanged.
The private tree remains the prior arm pack, hash `2929e8a6` (full proof in packet 3).

Inspected `build/draw-path-4/present-final/frontier.ppm`, 640×480:
`c7d02f642ae8e5493f19051e7cf13a06a2d1d103351e72b0576a108b52aba427`.
The image shows the blue outgoing Sonic Adventure DX title with the yellow logo,
Adventure text and Director's Cut subtitle. No CharacterSelect scene or actor is
visible. The unchanged **drawn-mode** checker exits **1** with
“outgoing title image is not a CharacterSelect display frame”. Its title guard runs
before reference-signature comparison, so **no distance is computed**. Frame 1450
was not reached; no actor-frame or N3b acceptance is claimed.

Reproduce with a new output directory:

```sh
python3 reference/packets/gamma-display-draw-4/capture.py build/draw-path-4/new-capture
```

`capture.py` records the exact unchanged checker command, exit, application hash,
final state, image hash and source-save hashes before/after. It requests frame 1550.

## Source and arithmetic proof

`SOURCE.json` retains exact PE bytes, disassembly, constants and the fresh Ghidra
body/pseudocode summary. Owned-runner dossier:
`ev_5fbdb37c43bdf5ca10ee640de4d0a5c75b359793d116fe44bd3bd9999ce15bf7`.
The body spans `0x775630–0x7756f6` exclusive, with an unreachable alternate-entry
prologue gap at `0x77563e–0x775641`. Whole-span SHA-256:
`b683a4b108b49e3524a4aee30b1a02b45e25503a34355bab9415ccc717aec9b7`.

The entry pushes EBX before reading out/input at `[EBX+8]`/`[EBX+12]` and returns
with `RET 8`. It captures all three inputs, squares/adds X then Y then Z, compares
threshold against squared length with `COMISS/JBE` (unordered takes refinement),
and uses `RSQRTSS` bytes `f3 0f 52 db`. The three compiler constants are:

| Address | Little-endian bytes | Literal |
|---|---|---|
| `0x3d13180` | `00 00 80 28` | 2^-46, `1.4210855e-14f` |
| `0x3d13184` | `00 00 00 3f` | `0.5f` |
| `0x3d13188` | `00 00 40 40` | `3.0f` |

The recovered engine routine preserves the interleaving of `length*r`, `0.5*r`,
then `(length*r)*r`, subtraction from 3, and final scale/output multiplies.
It supports in-place normalization and writes each component after its multiply.
The lighting adapter writes the borrowed light direction directly and borrows the
display's existing SSE context. No temporary hides a partially written destination.

The new named CPU helper measures/models **Unicorn 2.1.4**: rounded binary32 square
root followed by rounded binary32 division, with selected rounding/DAZ and discarded
estimate flags. Each cohort executes **32,176 isolated RSQRTSS fixtures**, including
all four rounding modes, DAZ/FTZ, subnormals, signed zeros, NaN/inf and finite random
encodings: **zero value or status differences**. This is not a captured physical
Intel approximation. Existing `sse_math.*` remain byte-for-byte unchanged; the
helper is separate to avoid stale fingerprints in unrelated Input proofs.

## Differential results

Whole original body executes without seeded/skipped leaves. Return pointer, stack
pop and preserved EBX are checked. Inputs include zero, signed zero, subnormals,
below/equal/above threshold, large values, NaN/inf on each axis, randomized finite
vectors, four rounding modes, incoming sticky flags and exact output/input aliasing.

| Cohort | Cases | Value/control failures | Flag differences | Strict exit |
|---|---:|---:|---:|---:|
| Development | 4,096 | 0 | 2,008 | 2, partial |
| Separate holdout | 12,096 | 0 | 6,046 | 2, partial |

Unicorn does not publish the SSE exception flags raised by native arithmetic;
these differences remain partials. Neither cohort is called a strict pass.
Three separately compiled faults are detected in development values: threshold
changed to 2^-48 (**48 failures**), missing refinement (**621**), missing Z store
(**3,740**). All fault libraries compile and their oracles exit 1.

The focused native unit passes exact-alias equivalence and verifies an unmasked
invalid square retains the untouched destination and sets its native invalid flag.
That unit is not differential qualification of all unmasked/partial-store behavior.
The existing lighting receipt oracle retains **1,597 exact values / 11 inherited
FP partials**, exit 1 under that judge's older exit convention. Its probe only
adapts the new destination/context signature; cases, receipts and expectations stay
unchanged. Two independent reviews and a fresh final recheck found no blockers.

## Commands and remaining limits

```sh
reference/python/bin/python reference/packets/gamma-display-draw-4/qualify.py
make -j2 -o Makefile oracle-normalize
python3 tools/oracles/verdict.py build/oracles/normalize.json
python3 tools/gate/track_reports.py normalize
```

`qualify.py` records exact argv/exit/logs under `build/draw-path-4/qualification`.
All builds use `-j2`. Focused builds mark Makefile old because only explicit new
recipes changed, and mark the existing verified SDL header old to reuse its build.
Source `sse_math.*` fingerprints remain unchanged. Profile checking of the new
range exits **0, zero unhandled intersections**.

`make -j2 check` passes source checks (including layering, provenance, readability
and constant-table reviews), then exits **2 at 71 protected-judge registrations**.
The orchestrator owns admission. Neither the judge manifest nor `tools/gate/*`
was edited. The full gate, full suites, cold source-only build, Windows CPU/raster
parity, later Gamma tail, completed/repeated actor frame and cancel/reentry were
not run. Canonical imports and pinned source installs are unchanged. Generated binary,
image, dylib and snapshot artifacts stay under ignored `build/`.
