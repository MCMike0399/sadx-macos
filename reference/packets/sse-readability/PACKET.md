# Scalar SSE readability packet

Provided checkout, branch `packet/sse-readability`, base `db2f1588122acf6aeb5e1cfa7484ab4da09904c3`.
Scope: complete `src/cpu/sse_math.hpp/.cpp` module. Header is already conforming;
retain its API/layout and implementation-independent comments. No Input/CRT/x87
source edits, Main writes, shared builds, network, delegates or full gate.

## Context before source edits

CPU-SSE-1 in review646f90f (`docs/refactor-inventory/reviews/cpu-sse-sqrt.*`)
requires naming/expanding the compressed body. STYLE7/12 and ARCHITECTURE retain
this narrow CPU numeric model: caller-selected rounding, NaN priority/payload,
signed zero, DAZ/FTZ, immediate exception priority and status are observable.
Input provenance and pinned SoftFloat3e revisiona0c6494 give the numeric evidence;
all107 local source/dependency pins and upstream hash checks are recorded in
baseline-source-pins.json. Source is evidence, not a new arithmetic specification.

Every public method remains in the existing SseContext. SseError carries its
unchanged failure kind, exceptions and text. Thread ownership precedes validation
and TLS changes. The private commit first merges new sticky flags, then tests only
new flags against masks. SoftFloatOperation saves/restores shared rounding,
tininess and flags on success and unwind; it never changes ext80 precision or
host fenv. x87 uses those same TLS symbols with its own scoped precision/control.

Binary arithmetic uses destination-first8086-SSE NaN propagation. Invalid/zero
divide suppress the lower-priority denormal fault. Unmasked denormal stops before
later arithmetic flags are published. Narrowing and widening commit operand
faults before entering SoftFloat. Roots preserve their distinct existing tails.
Single finishing and SUBSD detect exact tiny results for unmasked underflow,
suppress precision there/for unmasked overflow, and apply masked FTZ with signed
zero and UE/PE. CVTT ignores rounding and has no denormal operand exception.
COMISS/signaling less-than invalidate any NaN; UCOMISD invalidates only SNaN.
MINSS invalidates any NaN and returns source bytes, also choosing source on equal
signed zeros. All operations commit before return: failures retain caller targets.

Direct consumers: input_mod_axes (ordered integer/length/normalization/conversion
and immediate source stores), input_mod_pad (trigger/poll/half-press), input
(sample_tick), math/d3dx_math (lane operations, matrix/inverse/reciprocal),
engine/ninja_projected_depth and ninja_view_native (mixed SSE/x87 projection),
runtime/native_input (borrowed context/normalized device path). Headers also reach
screen/view, logo_renderer, native_display, CRT floor, reference overrides/HLE and
Mac diagnostics. Those algorithms and their early branch bypasses stay unchanged.
Current Main Input packet and its augmented preciseY-CVTT contract were read
before owning SSE. Its expected327681/24903828/FNV9340279443059284959 stays pinned.

## Before-source evidence and commands

Only this checkout's ignored build/sse-readability is written. Both numeric
archives are rebuilt from local pinned thirdparty source, never borrowed from a
Main maintained archive. `build.py` compiles all maintained dependency TUs directly;
its build.json records every actual command and exit. The unchanged old SSE,
actual DLL axes and DreamPad judges are observed by packet capture wrappers;
inputs, expected values, masks, tolerances and classifications are untouched.

```
sh third_party/softfloat_sse/build.sh build/sse-readability/pinned/sse
sh third_party/softfloat/build.sh build/sse-readability/pinned/base
python3 reference/packets/sse-readability/build.py build/sse-readability/baseline
PYTHONPATH=tools/lib:tools/oracles /Users/burbujamc/Developer/sadx-macos/reference/python/bin/python3 reference/packets/sse-readability/capture_sse.py build/sse-readability/baseline/probes.dylib build/sse-readability/baseline/sse
PYTHONPATH=tools/lib:tools/oracles /Users/burbujamc/Developer/sadx-macos/reference/python/bin/python3 tools/oracles/capture_input_axes.py /Users/burbujamc/Developer/sadx-macos/reference/updated-pc-reference/game/mods/sadx-input-mod/sadx-input-mod.dll build/sse-readability/baseline/probes.dylib build/sse-readability/baseline/axes
PYTHONPATH=tools/lib:tools/oracles /Users/burbujamc/Developer/sadx-macos/reference/python/bin/python3 reference/packets/sse-readability/capture_pad.py build/sse-readability/baseline/probes.dylib build/sse-readability/baseline/pad-final
```

Raw libraries, full gzip streams and original reports stay locally pinned under
baseline/. Metadata records exact hashes and lossless per-case identity ranges.
SSE remains FAIL:312112 cases,5076 value disagreements, all classified (NaN320,
FTZ4220, MINSS/DAZ536), no unclassified cases. Axes109056 values exact/93104 FP
partials; pad1044 values exact/300 FP partials. Neither partial is a full-state pass.
Native axes contract exits0 with the pinned327681 cases/24903828 bytes/digest.
Registered old SSE unit exits0. New sse_math_contract fills the review's explicit
coverage gap before source changes: all15 arithmetic/comparison entries, each
rounding/DAZ/FTZ choice,8 mask configurations,2 sticky inputs,18x18 edge pairs,
retained results/messages, all18 public wrong-thread entries, setters/clear,
SoftFloat TLS/host fenv and interleaved x87. Its frozen stream is maintained-model
characterization, not original Intel evidence. No dedicated historical SSE mutant
receipt was found; downstream Ninja/CRT mutations do not become SSE proof.

Initial capture-pad development failed on signed Python input packing. Both failed
attempt logs remain local; corrected signed64 input capture preserves actual API
input integers. Neither error is counted as a test or mutation success.

## Remaining qualification

Source rewrite, exact stream comparisons, compiled semantic mutations and direct
consumer compilation follow in separate commits. Root owns independent STYLE12
review, reserved fresh final cohorts, judge manifest union, inventory ratchet,
actual source-only product closure, journeys and coherent full gate. No whole CPU,
Input, consumer, campaign, hardware, Windows or gameplay closure is claimed.
Reserved UNRUN final seeds: scalar0x53534591 and native0x535345d7; Root may instead
select private fresh seeds. They are never used during this packet's development.

## Completed source and qualification

Source commit `696739d` rewrites the entire implementation into named raw-word
classification, signed DAZ replacement, scoped SoftFloat state, comparison,
width-specific finishing, explicit arithmetic/commit/return steps and named
operation selection. All added internal structure is NOTSA. Header bytes and
public API/layout are identical todb2f158. The exact masks are numeric encodings;
no host arithmetic replacement, owner migration, inferred behavior or fix was made.

`development-comparison.json` records full decompressed byte equality for all five
streams (SSE16,229,824; axes30,497,802; pad609,696; nativeSSE199,068,120;
nativeAxes24,903,828 bytes). `compare.py` checks every byte plus every retained
value/FP/class identity, not aggregate counts. SSE retains5076 classified value
failures and232720 status disagreements, including all incoming controls and
returns. Axes retains93104 partial identities, pad300. NativeSSE1244160 edge
observations plus state/thread/constructor cases keep FNV4458530012893139995;
NativeAxes327681 including preciseY-CVTT keeps FNV9340279443059284959. Baseline
and current raw libraries/streams are pinned in this worktree, never overwritten.

Eighteen copied semantic faults are caught by the unchanged pre-source native
contract. `faults-initial.json` retains all18 attempts:16 valid/caught,2 invalid
compiles,0 survivors. The invalid rounding/denormal-priority edits made their
parameters/locals unused underWerror. `faults-corrected.json` retains two valid,
caught replacements that keep those syntactic uses. No invalid compile counts.
Faults cover selected rounding, NaN order, DAZ sign, FTZ, exact tiny unmasked
underflow, overflow PE priority, binary64 FTZ sign, denormal priority/stop,
incoming sticky trapping, sticky merging, CVTT precision, UCOMISD quietNaN,
MINSS source bytes, equal signed zero, TLS restoration and thread rejection.
No dedicated historical SSE mutations were found; the existing unit assertions
and unchanged judges remain intact. No new oracle expectations mask these faults.

`consumer-builds.json` records all8 actual direct maintained consumer TUs compiled
successfully with dependency files. `consumer-source-closure.json` pins48 actual
source/header inclusions and checks baseline/current equality. No consumer source
changed. `current-build.json`, `extra-units.json`, `current-contracts.json` record
optimized SSE, axes, pad, D3DX and native contracts exiting0; the same SSE contract
also passes UBSan. Only pinned thirdparty numeric archives are linked. The first
D3DX unit link omitted its real x87 transcendental dependency and failed; the
successful build compiles x87_transcendental.cpp directly and links the locally
rebuilt pinned Bochs math archive. This is a unit check, not whole renderer closure.

Readability, layer and unique-owner checks exit0. No whole gate was requested or
run. The constexpr checker exits1 with exactly2 expected stale modes/source pins;
`rounding-table-review.json` supplies original/current fingerprints and unchanged
four-entry semantics for Root reconciliation. Global inventories, manifests,
readability allowances, HANDOFF and STATUS remain untouched. All inherited
path entries remain in the committed judge manifest. Available protected bytes
are unchanged except appended Makefile/test registrations;53 ignored inputs are
absent locally and no missing corpus was synthesized/restored.

Author STYLE12 review: complete module has structured control flow, named masks,
one statement per line, direct narrow SoftFloat calls and no address transport.
Arithmetic/control/store order matches the frozen model, with every commit before
return. Per-expression arithmetic deliberately stays at the existing numeric
boundary; ordinary hostfloat cannot express these caller controls/statuses. New
helpers only name existing raw classification, comparison, finishing and scope.
No guessed source facts, new services or ownership change. Independent review is
explicitly NOT performed by this implementation worker.

## Reproduction and Root handoff

After the source commit, run the same capture commands with baseline replaced by
current and pad-final replaced by pad. Then:

```
python3 reference/packets/sse-readability/build.py build/sse-readability/current
python3 reference/packets/sse-readability/compare.py
python3 reference/packets/sse-readability/mutate.py
python3 reference/packets/sse-readability/mutate.py build/sse-readability/faults-corrected rounding-selection denormal-priority
python3 reference/packets/sse-readability/consumers.py
sh third_party/bochs_fpatan/build.sh build/sse-readability/pinned/bochs
python3 reference/packets/sse-readability/check_units.py
python3 tools/gate/check_readability.py
python3 tools/gate/check_layers.py
python3 tools/gate/check_global_owners.py
```

These capture/build/fault scripts refuse existing output directories to preserve
proof. Initial faults are pinned as executed, while the maintained mutation driver
uses corrected compile-valid spellings. Use new paths or a new isolated checkout
for a new run. Baseline files were captured before Source, except the later D3DX
unit dependency closure, which is clearly current-only. The pre-source SSE native
contract was packed/recompiled before capture, then its measured digest assertion
was added and verified before Source; characterization.json pins that final test.

Root next: independent complete-module STYLE12/evidence review; fresh final
cohorts against retained old/current libraries without using default failures as
pass; reconcile the modes table evidence and lower readability allowance; union
all inherited protected paths and new contract/packet helpers into the judge
manifest without dropping any53 ignored corpora; run actual source-only closure,
affected runtime/consumer judges and journeys, then the coherent gate. Those are
not worker qualification claims. Header/body together constitute this complete
module readability result; whole CPU/Input/consumer/campaign closure remains open.
No push, no Main mutation, no new hardware/Windows/gameplay qualification.
