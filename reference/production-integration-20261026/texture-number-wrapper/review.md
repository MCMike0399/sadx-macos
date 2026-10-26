# Independent review: actual texture-number wrapper

Verdict: bounded approval of `cb86c98` against `09421ac`. No blocking findings
in the new Direct3D_SetTextureNum wrapper, SpriteImmediate binding, new judge/probe,
or strengthened binding unit. This does not approve N3b completion or GPU parity.
Reviewed on `packet/review-texture-number`, 2026-10-26.

## Standards

The new engine function has a semantic name and source-entry annotation, uses
the existing canonical State references and cache implementation, and introduces
no service, owner, address call, executable dependency, or unexplained literal.
The symbol database and mod-loader SADXFunctions.h identify Direct3D_SetTextureNum
at 0x78d140. The header's void signature does not describe the tail-called EAX
result; retaining the actual cache result is supported by the source instructions.
The runtime method forwards through the existing renderer and binding seam.
The only new null-list branch throws an explicit Error; it fabricates no successful
return. Existing unknown palette/upload and device boundaries remain explicit.
STYLE section 12 inspection found no masked mismatch, invented behavior,
address-shaped game logic, or new abstraction in this correction.

## Specification and source evidence

I read Root's full cached dossier `reference/ghidra-game-loop-recovery/dbg-78d140.json`.
Its SHA-256 is `e9a67115669ae51c38347c1518206f586b8086cc06541775fb34e3ca1d0c2ea0`,
matching proof.json; evidence ID is
`ev_fb45801d594544b47490559aecd18b682f0bca06d60e77ba9a21eb47618ff9db`.
The five instructions compute ECX times 12 with 32-bit wrapping, load the canonical
current list, load its names pointer, read Memory at the wrapped offset plus 8,
then jump to 0x78cf20. There are no current, queued, or requested stores and no
forced return of 1. The native wrapper reproduces those effects with the existing
typed list entry and cache functions. Unrepresented array words fail explicitly.

Display::sprite_services now calls that wrapper, with signed short conversion
preserved. The caller ignores the result. Its preceding direct-list selection
still establishes requested memory from entry zero; choosing another frame index
does not replace that requested owner. SpriteImmediate still selects, draws,
then restores linear filtering in the same order. Cache/palette behavior and its
callback-before-ID-before-active publication order are unchanged.

## New judge audit

NumberSource reuses only the old fixture setup: its code hook redirects EIP at
0x77f440 before any old instruction executes, preserving ECX and the return stack.
It executes the real 0x78d140 body and reached original cache/palette instructions.
Fixture writes install Memory pointers or explicit null pointers in names Memory
fields; no source-body bytes are patched. Inherited executable loading copies
the original PE sections; the synthetic external device receipt is separate.

Null current stops at 0x78d149. A wrapped offset into an unprovided ordinal or
non-Memory field stops at 0x78d14b, rather than reading zero-filled filename or
attribute words as fabricated pointers. A supplied null Memory reaches the
original 0x78cf24 boundary. The C++ probe maps corresponding tagged Errors to
those boundaries. This qualifies concrete native resources and explicit unknown
boundaries, not arbitrary foreign pointer layouts.

I compared the new probe with the old concrete-core probe: changes are limited
to the new ABI/entry, wrapper boundary mapping, nullable Memory entries, and
callback failure classification from the selected memory rather than requested
memory. Source expectations come from original execution, independently of the
C++ result. Requested/active identities, cache, current/queue state, revisions,
calls, and callback-time state are compared; only an incomplete return value is
excluded. Directed inputs retain null lists, array limits, wrapped indices,
format results, receipts and null Memory; random requested/active/cache/queue
values vary independently. I found no narrowed old corpus or correlated
expected-value calculation. All previously protected manifest hashes are
unchanged; only the new probe and verifier are added.

## Inspected receipts and limits

Root's `build/oracles/texture_number.json` and `texture_number_holdout.json`
record 3,408 and 5,408 cases, seeds 781140 and 271828, with zero mismatches.
Their five recorded source/probe/verifier hashes match this reviewed checkout.
The actual baseline library hash matches both reports:
`05cc8ac88f308543ac63f834de6b0f836a5326dbf5df6af84923c4ce7e93cca5`.
Unchanged old judges record 683 and 3,669 passing cases; all their recorded
source hashes also match. The binding unit adds a non-1 format return and
preserved requested-owner assertion. It is a direct renderer assertion, not
an independently executed end-to-end Sprite journey in this review.

I read all five mutant source diffs, result records, raw mismatch reports and
judge logs under Root's `build/texture-number-mutants`. Compile statuses are
recorded as 0 and oracle statuses as 1. Mutant source and actual library hashes
match results.json and proof.json. Requested-write, wrong-return, stride,
null-success and current-clobber mutants produce respectively 590, 133, 346,
1,643 and 785 mismatches on the identical dev corpus hash
`6f2a3e5a8c902471d81f9acd7d976a3d137b403c3c0141e49647727a9d161144`.
Their report source_files hashes describe baseline Root files, not the mutated
compile input; the separate mutant source/library identities establish that input.
The older verifier hash `e7e305d9...` is exactly recovered by removing only the
Unicorn import and 2.1.4 assertion from the final script. Final baseline/holdout
receipts pin the final `5edd33e6...` verifier. No case-generator change is hidden
by that difference. The proof hash matches the judge manifest.

The prior review 043fa85 bounded its approval and left full 0x78d140 unverified;
this correction addresses that caller distinction. The stopped combined gate
has recorded exit 2, not a green result. The repeated full combined gate and
source-only build remain Root's pending integration work. Ordinary model
rendering and late/Euler paths remain seams. No CharacterSelect completion,
model rendering, physical GPU, or full N3b claim follows from this approval.

I did not independently rerun any compiler, application, unit, oracle, mutant,
Ghidra session, source-only export, or full gate, and started no workers.
This review used source/diff/dossier/receipt reads and hashes only, plus the
requested branch rename and this review artifact commit. No code was edited.
