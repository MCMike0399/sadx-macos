# Frozen basic-model core review

Reviewed `9220d1b0161f5f206ac56527f1530d01539b1e22`, including base
`16f880580d96444991c8826462b27e166d767566`, against `04c40ae`.
Independent read-only review of frozen git objects, cached original assembly,
producer artifacts and pinned PE bytes. Read AGENTS, GOAL, STYLE, STATUS, latest
HANDOFF and relevant basic-model/strip memory. No workers, builds, apps, Ghidra,
gate or tests run. **Tests not rerun.** Only this review and its bounded evidence
are authored in `packet/review-basic-model-core`.

## STYLE §12 checklist

1. **Address shapes:** engine code uses named operations, structured flow and
   annotations. No guest dispatch, fixture-knownness fields, executable parsing,
   emulator arithmetic or relocated game algorithm. Producer readability receipt
   reports zero across all ten metrics for both new engine files; inspected code
   agrees. That checker was not rerun.
2. **Names:** source declarations and the single evidence-backed `791a80`
   semantic override agree with the cached bodies. The old object-draw seam loses
   its duplicate source-entry annotation without changing its interface.
3. **Order/quirks:** checked all 13 assembly dossiers, including the full material
   parser. Byte-valued colors remain unnormalized; all 17 device-material words,
   raw exponent, address-mode calls, independent destination bits 26–28, ignored
   device HRESULTs and texture-number call match the source. Environment-off
   reset is reachable at `784aa9`. Saved entry flags govern branch decisions;
   subsequent flag updates read live state after environment API calls.
4. **Arithmetic:** named ExactFloat expressions retain offset alpha, spill the
   other normalized inputs, clamp in source order and convert B,G,R,A with low-byte
   wrap. Quiet constant comparisons retain B,G,R,A short circuiting. No unsupported
   plain-float simplification or Question-entry precision premise was introduced.
   Existing FP-status discrepancies remain partial.
5. **Constants/content:** independently checked the actual EXE SHA-256
   `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`
   using PEImage. All 12 data ranges match stored bytes/hashes, including address,
   blend and primitive tables, environment/reset matrices and reset-world source.
   Independently decoded the existing Question pack: model 2943 uses material
   2949, words `ffffff00 ffffffff 41300000 00000007 9461a400` (power 11).
   Environment mapping is reached. Meshes are immutable borrowed resource views.
6. **Unknowns:** alternate, other primitive and buffered paths retain explicit
   stops; no successful fallback or extra cleanup was inserted. Lower D3DX,
   texture/cache and strip algorithms are receipt seams in this judge. Environment,
   reset, offset and shading bodies really execute original instructions.
7. **Judge integrity:** frozen diff adds new judges rather than changing existing
   expected values/tolerances. Original oracle hash is
   `d4d92a0efaf206c07a4113cdc1dcd17b78ffd9236f97bb846938fe2a72f290e0`.
   Algorithm, oracle, runner and manifest hashes match the frozen base; both case
   binaries and full receipt files match the base hashes. Seven mutant sources,
   compiled-library hashes and value-failing verdicts checked; no compile failure
   or flag-only discrepancy counted as a kill.
8. **Extension seams:** original material callback and ordinary link-time device
   seams provide the relevant hook points without adding a speculative registry.
9. **Invented facts/abstraction/owners:** nine scratch ranges are independently
   verified virtual zero fill, with four distinct matrix owners. Canonical state
   remains borrowed; no graph copy, service hierarchy, guessed identity or new
   specular-power owner. `Owners.current_matrix` now has the canonical float-array
   type. Each numeric seam memcpys its representation per call, preserving bits
   without aliasing or a cached shadow. The engine rereads through that reference
   for each operation. Reset-world remains the distinct `389d318` owner, not world
   scratch or device world; inverse view remains `389d358`. The documented
   CorrectMaterial power binding agrees with SDK SP_pow at palette offset 48.
10. **Platform boundary:** no macOS device replacement is implemented here.
    Reference-only synthetic backend behavior stays in probes. Actual native
    device bindings and their platform review remain integration work.

## Verdict and findings

**Approve the bounded core and current-matrix borrowing correction. No blocking
STYLE/fidelity defect found in the reviewed scope.** This approves neither a full
renderer nor native runtime integration.

- **Low, documentation:** `tools/oracles/verify_original_basic_model.py:3–4`
  still describes shading/environment/material-offset as receipt-only callees.
  Its RANGES and code hook actually execute these original bodies; PACKET and
  report limits correctly describe that stronger coverage. Update the introductory
  docstring to distinguish these bodies from D3DX/texture/strip receipt boundaries.
  Criterion: honest proof scope (AGENTS “Report honestly”). No behavior change or
  judge expectation change is needed; this does not block the core.

## Evidence and limits

`REVIEW-evidence.json` records independently verified hashes and recounts. All
13 dossier identities/source hashes, 13 original executable ranges per report,
10 frozen source hashes and seven raw-output hashes check out. Independent
intersection of full function ranges with active/undetermined profile patches
finds zero overlaps, agreeing with the producer's 0 UNHANDLED receipt.

Recounted every raw receipt: **2,427 dev / 3,427 holdout; zero value/order
mismatches; 172 / 274 FP-status mismatches; producer exits 2, passed=false**.
Producer unit exit is 0. All seven compiled mutants have value mismatches and
exit 1. These are inspected producer outcomes, not rerun results. Deterministic
cases are shared between corpora; randomized suffixes use different seeds.

Callback assembly saves identity once before the mesh loop and invokes it after
changed material. There are 38 cases per corpus with repeated first-callback
calls despite mutation of the global hook; the reload mutant fails 38 cases.
API mutation also exercises live render-flag updates. Synthetic matrix inputs
and reset-world values are distinct, and the unit changes current matrix after
Owners construction. Actual CorrectMaterial behavior is separately reviewed,
not established by the synthetic callback.

Correlation limits remain explicit: native/reference adapters share synthetic
lower algorithms and fixture inputs. These receipts establish operands, selected
calls, ordering and observed state, not D3DX numerics, texture-cache behavior,
strip conversion or GPU output. Buffered words are always zero in this corpus:
its stop is code-inspected only; no Buffer differential case exists. Original
unmasked exceptions and complete intermediate store prefixes are not proved.

Root must bind the actual live owners/device, stable callback identity,
CorrectMaterial and independently reviewed strip; register new judges separately;
then perform native/runtime/GPU and composed journey verification. The prior
Main gate's 262 native / 386 Python results exclude this packet and certify no
new component or N3b. InitPolyBuffers ownership is outside this review.
No push. No source changes requested beyond the optional docstring correction.
