# Independent spec/fidelity review

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
