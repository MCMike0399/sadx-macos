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
