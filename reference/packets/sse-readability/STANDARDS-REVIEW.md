# Independent scalar SSE standards review

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
