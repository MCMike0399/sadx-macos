# Gamma arm and ordinary display tail — presentation still blocked

Branch `packet/draw-path-3`, base `f4e5fc2`, implementation `987e176`.
The real constructor/display submits **106 draws / 3,060 vertices**: the prior
98 draws / 2,862 vertices of the animated body plus **8 arm draws / 198 vertices**.
The final arm motion consumes three nodes. This packet does **not** present an
actor frame or certify N3b.

## Private asset proof

The pinned archive's export 50 is action `0x1025d73c`, root `0x1020bcac`, motion
`0x1025d730`: three animated nodes, 20 frames, type 7, MDATA3 layout 3. The importer
adds position/rotation/scale tracks and inverse-proves all source references.
Private SDXACT03 adds Tracks3 kind 15; startup/title schemas retain their old kind
limits. Imported and generated motion views now resolve 24-byte rows and count
words at +12. A keyed generated-scale test detects the previous MDATA2 count offset.

The canonical 3,328 records / 766,112 bytes reconstruct to
`c414e173bf4bf9f803f24d61642e233ff9650fe9276d70da2ac3939fee2b5d7f`.
All 22 bindings, old record IDs and payload bytes remain unchanged. The existing
3,497-record display appendix reconstructs to
`382e3e53b613c42890c9e48afa3fae3e4d9398e6520019a1a2150451422688dc`.
Arm records append at IDs 3498–3537. The final private pack has 3,537 records,
787,208 bytes and hash
`2929e8a6a9da83b5f2f7f533d21b4d87036e017d36da3773e81ffef9a7881b5e`.
Declaration 50 and its recipe aliases are loaded. The source-initialized nine-object
flag list borrows existing scene objects; it adds no further records. All 3,537
records pass inverse/round-trip proof. ASSET-PROOF.json retains the commitments.
Canonical `game/imported` and the pinned source installs are unchanged.

## Recovered source and actual boundary

The ordinary body clears the Ninja callback, places/draws the arm using its saved
E102WK frame, reloads that frame/action for CalcMMMatrix(node 2), then pops the arm
frame. The inactive tail clears the actual flag-object list, computes hip, laser
and gun points through maintained matrices, preserves scratch Y, updates arm aim,
eye height and headlight alpha, and restores attributes, material, control and
palette. Laser mode borrows the maintained typed extra-work owner. Supplied matrix
pushes preserve capacity-failure behavior. Active hover/gun/laser/death/paused,
event, walking and hidden branches retain explicit named stops.

The final normal-input run requests frame 1550 with fresh copies of the three
hash-verified saves. It exits **1 at frame 1332**, completed frames **1330**, while
restoring palette 0: source call `0x4806fa` → ___dsSetPalette `0x412420` →
ApplyLSPalette `0x40a630` → unrecovered D3DXVec3Normalize_SSE `0x775630`.
`native_lighting_palette.cpp` explicitly throws at this existing seam.
The exception precedes frame completion and presentation. No cleanup call was
skipped to force presentation.

Inspected capture: `build/draw-path-3/final/normal-input/frontier.ppm`, 640×480,
SHA-256 `c7d02f642ae8e5493f19051e7cf13a06a2d1d103351e72b0576a108b52aba427`.
It is the outgoing blue SONIC DX title; no CharacterSelect or body colors/geometry
are visible. The drawn-goal checker exits 1 with “outgoing title image is not a
CharacterSelect display frame.” Reference distance is not evaluated because that
guard rejects the title first. Frame 1400 was not reached. Original save hashes
remain unchanged. The actual caller, whose palette service is a test stub,
completes display and stops next at `native_gamma_update_target_effects`.

## Focused judges

Every row has zero value failures. Existing development and holdout observation
digests are preserved, including the topology and placement cohorts.

| Cohort | Development | Holdout | FP partials dev / holdout |
|---|---:|---:|---:|
| Topology / placement, preserved | 1,126 / 1,024 | retained prior scope | not compared |
| MDATA2 DrawAction, preserved | 1,253 | 4,325 | not compared |
| MDATA3 DrawAction, new | 1,253 | 4,325 | not compared |
| Gamma callback, preserved | 1,024 | 2,048 | 13 / 13 |
| Ordinary pre-arm prefix, preserved | 1,024 | 2,048 | 90 / 90 |
| Arm calls and inactive tail, new | 1,024 | 2,048 | 680 / 1,364 |

The new tail judge executes original instructions through the arm call, matrix
producer call and reached tail, comparing ordered receipts and stores. Numeric
matrix, normalization, renderer and sine leaves are common seams; it does not
certify their numeric implementation or pixels. Qualification is limited to
masked arithmetic exceptions; detached output vectors do not qualify partial
stores under unmasked exceptions. FP partials retain strict exit 2.

Four new compiled faults are caught: suppressed arm 1,024 mismatches, matrix before
arm 1,024, missing control restore 595, missing callback clear 1,024. The five
unchanged prior faults remain caught with counts 1,249 / 1,176 / 1,228 / 154 / 402.
Independent body and arm suppression abort the real caller: respectively eight
and 98 remaining draws, both process exit -6. Five focused native programs and
18 Python tests pass. Three independent reviews found corrections recorded in
VERIFICATION.json. The generic verdict tool prints an inherited schema failure
for the topology report (“no recognised result key”); its original judge exits 0
and retains zero failures and its digest. No verifier was changed to hide this.

## Commands and limits

`qualify.py` and `final_checks.py` retain exact argv, exits and logs under
`build/draw-path-3`; VERIFICATION.json records them and the report hashes/counts.
All make builds use `-j2`. The inherited SDL header target is marked old because
this worktree already has the verified built SDL dependency and `cmake` is absent.

`make -j2 check` passes source checks and exits **2 at 66 judge registration
violations**. No judge manifest or gate tool was edited. The two unchanged schema
descriptor tables retain their declaration hashes; only source-file evidence hashes
were rebound after decoder edits. Range checking exits 1: four report intersections
are the same inherited active LinearMotionA_r hook, tested only with interpolation
disabled. The orchestrator owns admission.

No full gate, full suites, cold source-only build, Windows raster comparison,
completed/repeated actor frame, cancel/reentry or N3b qualification ran. The next
product dependency is stage-direction normalization during palette 0; the common
Gamma tail also retains target/gun/held/ground/collision continuation boundaries.
