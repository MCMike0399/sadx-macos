# Independent texture-list gate review

Decision: APPROVE the bounded recovery packet, with no blocking findings.
Round 1; no prior findings. This does not approve runtime composition or certify a journey.

Reviewed commit: `f600f578d00e23ad7f4fdcaca58d2ca65d02667b`.
Base: `3878f33d8179a0ea9805655c726dfaad9c5de4c0`.
Diff: `git diff f600f57^ f600f57`, all 11 changed paths.
Review branch: `packet/review-texture-list-gate` in its separate worktree.
The implementation worktree was read only during review.

## Fidelity

The complete cached dossier is `reference/ghidra-action-fp-recovery/sprite-403250.json`,
evidence `ev_8a080598de70eac5bd22d0f4bb023ad47a7ea2a6a50e72b41ac7b622040fae91`.
I read its assembly and pseudocode, verified the dossier hash
`588941b87597ccba49904161d9fca4531ad170631dd43db364a1e55c80daa14f`,
and independently read the pinned PE using PEImage. The subject hash matches
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`;
the full 61-byte range `[0x403250, 0x40328d)` matches
`6df189d84533a30623a99b4e6c68bf5a4b5053abcfb8acf19899035bac7d7eb8`.
The source has no callees or mutable globals.

Source null returns 1 without dereferencing the list. Nonnull reads count, saves
ESI, and loads the array pointer even when count is zero; zero returns 1 without
reading an array element. Positive count checks each binding at stride 12, offset
8, stopping at the first null. Only after all bindings pass does it read the first
filename and return its equality with -1. No surface or later filename is inspected.
The native typed function preserves these decision dependencies and true-not-ready
polarity. Its vector and sentinel are validated import facts; it does not claim to
reproduce invalid guest pointers or the original layout's otherwise redundant read.
The oracle explicitly checks that original read order, stack-only writes, ESI and
return ABI while executing the original bytes without replacing this gate.

`list_not_ready` borrows canonical `texture_lifetime::List` and actual Memory
bindings, including aliases and bindings with no live surface. No owner, service,
layout surrogate or fixture optional is added. The header and packet explicitly
require complete count/array/sentinel import facts. The unchanged legacy
`ninja_texture_list_gate` still reports unknown list/count/array/reference/filename,
short array and null array at their source boundaries. Those facts have not been
turned into fabricated native defaults or hidden by moving code into Guest.

The LateSprite hunk is an exact extraction of its existing list checks. Loop count
and null sprite still precede the list read. There is no callback inside the new
gate, and every subsequent priority computation, execution-mode guard, callback,
material/depth access, failure and type-2 unknown boundary remains in place.
The call introduces no earlier snapshot across callbacks and no extra Memory read.

## STYLE section 12

The new header and cpp contain semantic names and structured control flow, with
all ten readability metrics independently measured as zero. No address-shaped
behavior moved layers. Existing NinjaSpriteQueue debt remains where it was;
message_sprite_queue retains its two existing emulator-arithmetic sites.
`isTextureNG` is confirmed by pinned SADXFunctionsNew.h lines 160-170, including
EAX argument/result ABI. The override explains the parser miss and naming choice;
the FUNCTION annotation is unique and on the declaration.

No new arithmetic, numeric constants, asset reads, platform replacement, STUB,
silent no-op, invented guard or speculative abstraction is introduced. The one
new function recovers an actual source function, so no NOTSA helper is needed.
Existing seams remain available to callers; no enhancement behavior belongs in
this small fidelity predicate. Existing judges, fixtures, tolerances, manifest
and gate scripts are unchanged. The new oracle/probe/mutant fixtures are additions.
No Root runtime files or registrations were edited. Registration and the full gate
remain explicit Root integration work, as documented in PACKET.md.

## Evidence checked and limits

I read the probe, unit, new verifier and six mutation specifications, rather than
inferring correctness from green reports. The proof is
`reference/production-integration-20261026/texture-list-gate/proof.json`.
Both raw development and holdout reports match every summarized field and all
recorded source hashes. The available final probe library matches their common
SHA-256 `8d07f88bfc4782e50959b44497d6ad0ff7f963a9d514568e9eae1e1056ad9177`.
I independently regenerated the fixtures using only the generator functions:
4,256 development cases hash to `eda2ee11778ba66cf309970f9455d50aeb5fcce4027bf3827155d4402a00da24`;
4,096 holdout cases hash to `a8a899fe4ad7c1e53d3509b145a6e76265b52171cd3b7f316a941be8137cc755`.
Both reports record zero mismatches and pass. Counts cover 0 through 128, not the
entire uint32 domain. Native CPU flags are outside this BOOL API.

All six mutant source replacements independently reproduce the recorded mutated
source hashes. Raw reports match their library hash receipts, first failures,
full development corpus hash and mismatch counts: 386, 20, 1118, 37, 912 and 445.
Mutant binaries are not separately retained; their execution is receipt evidence,
not an independently repeated run. The final restored library hash was checked.

LateSprite before/after raw reports match the proof: 1,000 cases, seed 404660,
zero value mismatches, one flag mismatch, routes 884/75/29/12. Both remain partial.
The old probe/verifier and entire ninja_sprite_queue.cpp hash-identically to the
parent. The old late-sprite reports lack library/corpus hashes, so that execution
is also reported evidence rather than an independently authenticated rerun.

Independent read-only Python checks passed: readability zero regressions,
provenance zero errors, layers zero violations, and profile zero unhandled,
handled or undetermined overlaps for the declared source range. This is not a
claim that all active profile patches globally are handled. Git diff whitespace
check passed. The recorded unit/format results were inspected, not rerun.

No compiler, oracle/native execution, application, full gate, Ghidra process or
worker was launched. RuntimeBinding and the actual Question queued-list path are
unverified here; Root must test the actual canonical owner and polarity during
composition. No GPU, CharacterSelect completion or gameplay claim is made.
