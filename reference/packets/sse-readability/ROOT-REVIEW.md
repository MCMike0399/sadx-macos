# Root review aggregation

Candidate `6810ee7`; fixed base `db2f158`. Independent reports are retained separately.
No result below consumes a reserved cohort or qualifies Main integration.

## Standards

Candidate: `6810ee70b702573f1106f5b833f7bcc96dd73834`.
Fixed comparison: `git diff db2f1588122acf6aeb5e1cfa7484ab4da09904c3...6810ee7`.
Reviewed commits: `e072241`, `41114f4`, `696739d`, `6810ee7`.

**Disposition:** BOTH complete `src/cpu/sse_math.hpp` and `src/cpu/sse_math.cpp`
conform within this bounded scalar-SSE readability purpose. This is standards
acceptance of the maintained numeric model, not Intel correctness or integration approval.

**Hard standards findings:** none. Applied AGENTS, PROMPT-complete-refactor,
ARCHITECTURE and STYLE §§7/12, including ownership, names, provenance, sequencing,
unknowns and abstraction. The unchanged header exposes explicit raw words,
caller-selected MXCSR, tagged exceptions and one thread-bound owner; API/layout
bytes match the fixed base. The implementation names classification, masks,
rounding and finishing without guest addresses, fixture state or host arithmetic.
Added internal helpers carry NOTSA explanations. Structured steps retain DAZ,
operand priority, immediate denormal stops, sticky merging before trapping and
commit-before-return. Thread rejection precedes validation/TLS mutation;
SoftFloat rounding/tininess/flags restore on return and unwind, leaving x87
precision and host fenv alone. Comments describe retained policies without
claiming repaired hardware fidelity. The private division default is inherited,
reachable through the named Divide wrapper, not a new silent fallback.

**Heuristic smells (judgment calls):** no actionable findings. Width-specific
finishing and instruction wrappers are justified numeric boundaries; combining
them into a framework would obscure the deliberately distinct operation tails.
Raw IEEE words and masks are appropriate domain representations.

**Evidence and limits:** source/header/contract hashes match tracked receipts;
the contract is unchanged since the separate pre-source judge commit. Retained
worker archives and old/current libraries match all five local-artifact hashes;
all 48 consumer-closure entries match both base and candidate. Reviewed stored
five-stream equality and 18 valid caught-fault receipts, retaining two initial
invalid compiles. No stream decompression or test execution was performed here.
SSE retains 5,076 value failures and 232,720 status disagreements; axes/pad retain
93,104/300 FP partials. These are not passes. The modes table's two stale checks
remain Root reconciliation work. Reserved seeds remain UNRUN. Root owns final
cohorts, protection union, integration, source-only closure, journeys and gate.
No whole CPU, Input, gameplay or campaign approval. Only this report was changed;
no builds, providers, delegates, gate or push.

## Spec

Candidate `6810ee70b702573f1106f5b833f7bcc96dd73834`; fixed base
`db2f1588122acf6aeb5e1cfa7484ab4da09904c3`. Reviewed the exact diff and both
complete `src/cpu/sse_math.hpp/.cpp`, campaign brief, STYLE §7/12,
ARCHITECTURE, CPU-SSE-1 (`646f90f`), PACKET and qualification receipts.

**No introduced spec/fidelity defect found.** This accepts the bounded
behavior-preserving SSE rewrite against its frozen maintained-model contract.
Header/API/layout remain byte-identical; only the CPP changes under `src/`.

`sse_math.cpp:69` preserves scoped rounding/tininess/flags restoration on return
and unwind, without touching ext80 precision or host fenv. Ownership checks
precede validation/TLS access. `:193` merges sticky status before testing only
new exceptions; no return publishes a destination before a possible trap.
`:210` preserves destination/source order into unchanged 8086-SSE SoftFloat NaN
propagation, including payload selection. Operand versus arithmetic exception
priority and early denormal stops match the base. Conversion operand commits,
distinct root tails, CVTT's explicit truncation/no denormal fault, COMISS versus
UCOMISD versus MINSS, signed-zero selection, and `:131`/`:149` tiny-result,
FTZ and unmasked UE/OE precision suppression all retain their original sequences.
No eager validation, invented fallback or host arithmetic replacement appeared.

`development-comparison.json` records complete byte equality for all five frozen
streams. Independently compared stored old/current identity ledgers and checked
retained library/archive hashes; no decompression or execution performed.
SSE remains **FAIL**, with the same 5,076 classified value failures and 232,720
status disagreements across 312,112 cases. Axes retain 109,056 cases/93,104 FP
partials; pad retains 1,044/300. Native axes retain 327,681 observations,
24,903,828 bytes and FNV 9340279443059284959.

The complete new contract is unchanged since pre-source `41114f4`;
`characterization.json` pins its final version. Reviewed all eight successful
consumer compile commands/includes, covering input/axes/pad, D3DX, mixed native
projections, native input and logo rendering. Included maintained bytes match.
Fault receipts separately establish 18 valid semantic faults caught and two
initial invalid compiles; neither invalid attempt counts as a catch.

No builds, tests, oracles, mutations, fresh inputs, gate or shared-writer changes
were performed. Reserved final cohorts remain UNRUN. Stored observations do not
establish original Intel/Windows equivalence or whole CPU/Input/gameplay/campaign
approval; Root's explicitly retained integration responsibilities remain open.

Standards: 0 findings, no worst issue. Spec: 0 introduced defects, no worst issue. Reserved qualification and integration remain pending.
