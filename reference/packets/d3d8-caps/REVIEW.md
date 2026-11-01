# Independent complete-module review

Candidate `3cd12d5cb8caf628b218fed5088971624d62302d`; Source
`d1a7b426c846c2a4dbf0fa95bc4b7a68b3c1c1de`; before packet `349de60`,
Main `6ed0ebeb2dde6afd9967ec90ec7d72a28502dfb2`.
Reviewed immutable blobs/diffs and frozen local artifacts against AGENTS,
STYLE §12, PROMPT-complete-refactor and ADR-0009. No executions were performed.

## Standards

No actionable findings. Bounded conformance applies to both complete files:
`src/engine/d3d8_caps.hpp:1–14` and `src/engine/d3d8_caps.cpp:1–36`.

The public API and array type identity remain unchanged. Named raster/texture
offsets, HAL/WFOG/mask constants and the local little-endian writer explain the
wire encoding without executable addresses, register transport or hidden state.
The writer and shared producer have NOTSA explanations. Structured integer code
retains zero initialization and the same three ordered writes; all indices and
shifts are within bounds. No allocation, callback, owner migration, invented
recovery, framework or speculative enhancement hook is introduced. This is the
existing disclosed capability assumption, with no newly recovered platform site.

The only maintained consumers remain the HLE COM adapter and NativeGame reference
construction/reporting. Neither consumer changes; no behavior is relocated to
evade readability checks. Shipping-list exclusion is unchanged and establishes
no new product policy. Recorded static-check exits are zero; this review did
not rerun those checks.

## Spec / fidelity

No actionable findings. Bounded preservation is supported for the complete
two-file module, including its explicit unknown-GPU disclosure.

Independently verified BEFORE source/test/consumer hashes against Git blobs,
candidate source hashes against QUALIFICATION and frozen files, and unchanged
unit tests and consumers. BEFORE-OUTPUT and AFTER-OUTPUT have identical seven
capacity/null records and complete diagnostic strings. Reconstructing the three
little-endian words independently reproduces every one of the 212 wire bytes
and trailing sentinels. The diagnostic has 214 payload bytes (215 including NUL).
Both library artifact hashes match their respective records; their sources were
separate old/current inputs, not dependency archives.

MUTATIONS records three compiled faults caught by the unchanged exhaustive unit;
mutant source hashes and raw failure logs agree. Guest log/map hashes and pinned
archive hashes match. The map assigns both caps definitions to the fresh object,
not an archive member. The unchanged guest unit checks both COM calls, output
bounds, one assumption receipt and original mipmap positive/negative behavior.
The documented UTF-8 metadata inspection failure does not alter those raw hashes.

Complete output comparison is exhaustive for this zero-argument constant producer;
there is no random input domain or random holdout claim. Runtime/provider/Windows
GPU parity remains unproven. Root retains probe protection, canonical inventory,
actual Main game-runtime composition and coherent gate qualification. No reserved
inputs, builds, oracles, game/runtime runs, global refresh or push were performed;
no whole-campaign verdict is given.
