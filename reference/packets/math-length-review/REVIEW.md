# Independent math-length fidelity and design review

Reviewed 2026-11-01 by the isolated review agent on `packet/math-length-review-independent`.
Candidate: `packet/math-length-consolidation`, base `3efc9b7`, commits `e411eff`,
`b5b5447`, `8aacd17`, `337aaa9`. Candidate checkout and pinned `build/math-length`
were read-only. This review changes no implementation, judge, fixture, build or Main file.

## Verdict and actionable finding

**Bounded approval of the complete `src/math/ninja_length.hpp/.cpp` module**, including
its delegation to the specifically reviewed CPU operation and the changed legacy rotation
wrapper. No arithmetic or design defect found in those changes. This is source review
approval for Root's remaining qualification, not a completed integration or full fidelity pass.
It does not approve the whole CPU, player, math foundations, or refactor campaign.

**P2 — Correct the Sonic setup diagnosis and preserve the frozen Main fixtures.**
`JUDGE-SETUP.md` opening paragraph and `PACKET.md` unit/Root-next paragraphs mistake a
standalone compiler setup omission for an inherited project build failure. Both base
`3efc9b7` and frozen Main `92dbd07`, Makefile lines 457–464, already force-include
`tests/reference/actor_current_fixture.hpp` for the Sonic units/probes. The packet's
`build.py` and `units.py` omit that include. Their failure is real for those standalone
commands; it does not establish a failing registered unit. Root additionally reports that
Main gate `b59ae07` compiled and executed the Sonic unit; this review did not run or wait
for a Main gate. Do not repair the unit on this evidence.

Recommended integration: keep Main's frozen Sonic probe, unit, Makefile and fixture
adapter. Do not integrate the probe rewrite/probe hash refresh from `e411eff` merely to
repair this claimed defect. Retain the private rebinding as packet-only evidence if useful,
correct the packet prose and machine-readable limits (including `qualification.json`),
and reconcile the manifest using Main's frozen probe hash plus the separately reviewed
mutation-driver change. This requires no arithmetic implementation change. Reproduce the
Sonic comparison through Main's existing forced include during Root's canonical replay.

## Sonic observer equivalence, including null publication

I inspected the complete frozen probe, its existing forced-include adapter, the `e411eff`
diff, `actor_current.hpp/.cpp`, and the actual `run_sonic_player` publication sites.
The two observation bindings are equivalent for the probe's outputs:

- Initially each frozen `PublicationReceipt::written` and each private boolean is false.
- Every setter first stores into the canonical `actor_current::State`, then invokes the
  matching callback. Both callbacks unconditionally set the matching receipt to true.
- A primary/player store of null still sets the receipt. Testing handle truth would lose
  this distinction; neither reviewed implementation does that.
- Frozen `has_value()` and private output words 19–21 read those receipts. Both report
  zero for no publication and one for a publication of null, zero, or a repeated value.
- Frozen range `value_or(0)` and private output word 22 both read the canonical byte only
  after its receipt is set, and otherwise return zero. They retain the byte without extra
  boolean normalization. Partial/early failure therefore preserves the same prefix.

The probe initializes fresh globals, installs the observer before calling game code,
and has no later observer reassignment. Lambdas only set booleans; they neither mutate
canonical values nor throw in normal operation. Even when the project forced include
is present, the private observer merely replaces the fixture observer and private output
reads replace the now-unused fixture receipts. No game owner is duplicated.

This establishes semantic equivalence by code inspection. The packet's 28,800 comparisons,
zero stored mismatches, 2,840 FP status differences and 7,152 observer-fault mismatches
support the private observer, but are not a direct executed comparison against Main's
frozen adapter. No such replay was performed by this review; Root retains it. There is
no demonstrated need to change the frozen probe to qualify length.

## STYLE §12 and arithmetic contracts

Inspected both complete candidate length files, the complete `ExactFloat` definition,
old/new `rotation_vector_length`, and the old/new signed-root implementations. The source
assembly in `scalar-allocation.assembly.txt` establishes component loads/squares in x, y,
z order, then `(zz + yy) + xx`, the binary32 squared spill and signed-root call.

The CPU operation borrows three `const float &` components and the caller's live context.
Each named load is followed by its square before loading the next component. Exact-value
register operands retain the old raw multiply/add behavior. The sum is stored once to
binary32. The unchanged signed-root logic loads, performs the signaling m64-zero compare,
reloads, conditionally multiplies by minus one, computes sqrt and conditionally negates.
It returns the private retained `X87Value`, with no root store. The typed math wrapper
uses the existing `retained` constructor, adding neither a load nor a binary32 conversion.
The legacy wrapper returns that raw value directly. Exceptions propagate at the same
software-context operation; no new catch, context reset, fallback or extra owner is added.

STYLE arithmetic levels 2/3 are appropriate: multiple precision/rounding profiles and the
retained ext80 return rule out plain host floats. The raw access is primitive-specific,
inside ExactFloat, not an arbitrary public getter. CPU depends on no math/vector/game
record. Both existing public wrapper signatures and ExactFloat layout remain unchanged.
The sole njScalor provenance follows the arithmetic body; the typed wrapper is marked
NOTSA. No guest relocation, address dispatch, source constants fetched from PE, new state,
STUB omission, or speculative abstraction is introduced. A generic hook framework inside
this pure arithmetic primitive would not improve this change. Remaining address-shaped
player code lies outside the approved module and remains debt.

## Evidence checked and actual reach

I read `PACKET.md`, `CONTEXT.md`, `JUDGE-SETUP.md`, `baseline.json`, all keys and bounded
summaries of `qualification.json`/`mutation-binding.json`, and the complete compact identity
ledger. I also inspected the capture, comparison, contract, build, unit and mutation scripts.
No script was executed to compile, invoke a probe, evaluate the original, mutate, or run a gate.

Independent file comparisons established:

- All 17 `baseline2`/`current` full capture streams have identical gzip compressed payloads
  and trailers after parsing past metadata headers. This proves complete captured stream
  identity without processing gigabytes of expanded reports or relying on aggregate counts.
- Baseline/current identity ledgers are byte-identical, SHA-256
  `154cbd981f8958d6e852059c2110d0b6126f05fef5e845e40d340edd6f124a25`,
  and the tracked baseline ledger equals them structurally. Individual FP-partial sequence
  identities and native boundary/failure identities are retained.
- Both 66,742,260-byte native contract files hash to
  `03e767f358a7acf87cee5fdec60e42ad8109e8f1f56af6a6ec338b1315a02add`.
  They encode 1,112,371 records over 4,792 vectors and 84 profiles. The contract observes raw
  ext80, typed binary32/binary64 stores, retained equality, CW, flags and immediate X87Error
  details. Each main operation has 95,213 unmasked failures. Profiles use three precision
  modes, four rounding modes, all-masked and individually unmasked exceptions; they are not
  an exhaustive test of every mask combination, incoming sticky status, or hardware state.
- Candidate source hashes match the qualification. All 61 maintained-source hashes in the
  current probe build receipt match candidate source. All 14 recorded original evaluators
  match their frozen hashes. The manifest retains all 929 base paths; its only changed
  inherited hashes are the private Sonic probe and the relocated mutation driver.
- All 33 current link-map object tables contain no borrowed archive member matching a
  maintained source. Their 105 distinct borrowed members are SoftFloat/Bochs code. Actual
  length symbols resolve to current length/rotation objects, including the shared inline
  primitive. This is a current-source isolated closure, not proof of a fully rebuilt Main app.

The 17 configurations retain their existing outcomes, not seventeen full passes.
Eventground and eventair remain script exit 1 with 12 nonflag case/CW failures each;
player_reaction remains exit 1 with 69 such records. Their full identities are unchanged.
All recorded FP partials remain partials, including collision_component exit 2 with 6,462
FP partials in 7,964 cases. The scalar subset is 444 length plus 444 signed-root comparisons,
not 888 length-only cases. Sonic's exit 0 tests stored state and does not erase its FP partial.

Source/link reach is distinguished from execution: stock contact-initializer, Sonic and
default attribute4000 alternate suites observe zero original length entries. Other captures
observe actual length calls, including rotation, position/contact, event branches,
attribute20000, mesh query and scene broadphase/position/helpers. Recorded actual native
Gamma binding and mesh-registry units add composition checks. Eighteen unit programs
build/run with zero exits in both frozen/candidate receipts; the standalone Sonic unit was
skipped after its include omission, as corrected above. This review reran none of them.

## Historical mutation preservation

`8aacd17` relocates the same exact sum-order token to `fidelity_float.hpp`; the token
occurs once and the retained private header differs by precisely that substitution.
The driver's private `include/cpu/fidelity_float.hpp` path and earlier include directory
cause both the probe and source TUs to see the fault. Other mutations and the evaluator,
fixtures, tolerances, cases and holdout behavior remain unchanged.

The old-location, relocated-location and actual-driver capture files have identical
compressed payloads/trailers (payload SHA-256
`8dfdfc8b21ea5bf05f0beb3a883184b610f900e0e976bd010fbe85ff821907aa`).
The actual-driver report retains 7,818 cases, 45 value mismatches and 6,342 FP partials;
its separately retained dylib is 227,352 bytes, SHA-256
`bcc24fa06dbc421fded22a9c406b7143e73f3b3ff25188c51841f5c28d3e1f1a`.
Together with the source inspection, these artifacts support the recorded actual patched
nested build exercise, rather than only a separately handwritten mutant build. I did not
reexecute that build or its automatic holdout.

The two additional faults discriminate the other contracts: eliminating the squared spill
produces 365 value mismatches in the numeric judge and 446,239 changed native records;
rounding the returned root changes 153,400 records. Retained finite raw-return examples
show why a binary32-only observer cannot qualify the return. All three faults are caught.

## Remaining qualification belongs to Root

Final fresh holdout, integration, frozen Main Sonic replay, full native rebuild/closure,
journeys, gate and inventory/ratchet updates remain outstanding. The packet's judge check
exits 1 for 53 unavailable inherited frozen corpora; it is not a passing gate. Do not drop
those hashes. Original FP discrepancies and event/reaction failures remain open. Full
hardware x87 stack/tag behavior and deferred exception delivery remain unverified.
No new visible gameplay, Windows hardware parity or whole-campaign completion is approved.
