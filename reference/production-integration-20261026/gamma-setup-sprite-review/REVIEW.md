# Independent bounded review: Gamma setup sprite constructor

Bounded approval of `da73181f37ae2b630f2ed659220d2fdb270bddb0` against
`cf1c51a730ab7dad7e9c4ef92003d34363e52366`. No blocking findings within the
portable constructor contract. This is the first independent review round.
Review branch: `packet/review-gamma-setup-sprite`.

Tests NOT RERUN. Inspection only: no builds, tests, oracles, apps, Ghidra,
providers, subagents, Root Main changes, shared writes or push. The companion
`evidence.json` records inspected identities and copied receipt observations;
those observations are author results, not new execution evidence.

## Standards

AGENTS.md, GOAL.md, STYLE.md including §12, STATUS.md, the last shared HANDOFF
entry, and JUDGE.md were read. All 20 candidate paths are additions. Existing
source, judges, manifests, baselines, recipes and shared HANDOFF are unchanged.

The engine core has structured control flow, plain unsigned arithmetic, semantic
names and no address-shaped dispatch, fixture optionals, Services/vtables or
Session/app changes. Head and count each have one source-address owner in State;
a candidate-tree source search finds no old or competing owner. SDK
SADXVariables.h:2328 names SpriteCount. RegisterSprite and SpriteRegistryHead
remain justified role-name proposals, not falsely attributed SDK symbols.
Canonical symbol registration remains Root owned. The private-symbol provenance
receipt is conditional and does not constitute canonical registration.

The plain link-time allocate_zeroed boundary borrows the actual TaskWorkArena.
It has no fake allocator implementation in the product. Missing fixture facts
and allocator diagnostics belong at its binding, outside the constructor.
The new header/core add no visible lexical readability debt by inspection;
recorded zero regressions and zero layer/owner violations were not rerun.
No platform replacement or new enhancement interface is introduced.

## Spec and source fidelity

Both Root dossier hashes match source-evidence.json, and both packet assembly
transcriptions match the normalized dossier assembly exactly. The constructor
is the complete 81-byte range [456c50,456ca1); [4c5420,4c5436) is context only.
All seven source/judge SHA-256 identities declared in proof.json match the
candidate Git blobs. The companion evidence hashes every added path.

The core preserves the null-definition early return; CAlloc(1, extra+28) with
32-bit wrap; null allocation return without constructor publication; live count
then head reads after the allocator callback; wrapped count increment; next,
head, count, conditional extra, then definition stores; and returned identity.
TEST/JBE skips only zero extra, including high-bit inputs. Source assembly,
rather than the decompiler's apparent early global increment, determines order.
Allocator callback mutations remain visible on both success and failure.

Node is the canonical typed representation of constructor-owned pointer fields.
Arena storage supplies allocation identity, extra storage and untouched bytes;
it is not a second numeric copy of those pointer fields. The probe overlays
native identities only when serializing this representation for comparison.
The unit explicitly sees zero raw header bytes after typed publication. This
approval therefore permits typed constructor use only: an overlapping raw-header
consumer must not be bound as if those bytes were synchronized. Future consumers
need an explicit coherent mapping or rejection policy. Wrapped-small allocation
and interior identities are represented, without authorizing out-of-bounds access.

make_shared adds host representation allocation after source CAlloc. The contract
explicitly excludes host bad_alloc from source allocation-failure qualification;
it propagates, rather than manufacturing successful construction or source NULL.
Lower CAlloc behavior remains intercepted and unqualified by this packet.

## Judge and receipts

The new source judge pins the original PE and constructor range hashes, asserts
source reads and ordered stores, leaves definition bytes unmapped, and compares
normal compiled native results. Its separate instrumented source copy records
five logical prefixes without adding a product observer or service interface.
The head/count reversal mutant retains final values but changes those prefixes;
the recorded receipt reports it caught by mismatches with exit 1. All seven
mutants report actual mismatches, not compiler failure.

Receipts report 1136 development and 2048 holdout cases, zero mismatches and
matching prefixes; seven mutants caught; actual-arena unit and source-only probe
build exit 0. Both complete context ranges have zero UNHANDLED overlaps in the
profile receipt. The receipt's 79 active unhandled patches elsewhere are not a
claim of whole-profile completion. The holdout is separately seeded and marked
untouched; its execution history cannot be independently reconstructed from these
receipts, and it was not rerun here.

The prefix method masks fields before their designated source store because
wrapped allocations may contain uninitialized bytes; final untouched storage is
compared. It is not exhaustive proof against every premature-store mutation,
arbitrary missing-memory fault, host allocation failure or native fault delivery.
Manual source inspection confirms the submitted order. No existing judge was
weakened, and these limits must remain attached to the constructor verdict.

## Approval boundary and Root follow-up

Approval covers this portable constructor witness only. Definition is genuinely
opaque. No dummy Sprite, guessed resource, duplicate numeric payload, definition
readiness, whole GammaSetup implementation or new app frontier is approved.
4c5420, definition/resource 9820f8, pointer 982148, result owner 982154 and binding
remain absent. Source-backed definition consumers/layout and an exact import are
required before Root binds the real Gamma call. Root also owns canonical symbols,
compiler/unit/judge registrations, the allocator binding and explicit boundary
failures. These are documented integration requirements, not completed actions.

The actual app1/frame1382 stop at native_gamma_load_setup_resource after qualified
WeldCursors is supplied context, not a new observation by this component or this
review. No PJoin, renderer, actor, GPU, N3b, story or full-gate approval is given.
Root's pending GUI gate retry is not certified here; the earlier failed gate
remains failed. Shared STATUS/HANDOFF were not rewritten.
