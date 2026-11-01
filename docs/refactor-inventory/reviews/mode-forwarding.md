# Independent mode forwarding qualification review

Candidate `0806b909bc9be7d84367ca3c3c89191aadcf47cd`, base
`353d528bbb1a50e8cb0e4cdeaf7657dc9ad8607d`. Baseline
`d860512b7524f3f4e285182c3fa103e0ea63a1c5`; separate judge commit
`697c9c257a37b9ee50782f34bad74574306c26f3`. Reviewed independently on
2026-11-01 in the provided isolated review checkout. Candidate files were read
through immutable Git objects; retained artifacts were read from the supplied
candidate's `build/mode-forwarding` directory. No candidate or Main writes occurred.

## Verdicts

**Source conformance: approve exactly two complete files / two stem modules within
their bounded native forwarding and explicit-stop purpose.** This replaces the
open qualification of those two files in review `7e96ba2`; it does not expand that
review's other dispositions. No source rewrite is needed to qualify this boundary.

| Complete member | SHA-256 | Disposition |
| --- | --- | --- |
| `src/game/game_mode_native.cpp` (275 lines) | `cdf3a649e194591c4ed3ec324cb6ae3ecac6c25c3ea83ec7c5a3e69a37e44dd5` | Conforming native state access, finite service forwarding, explicit missing-system stops and evidenced diagnostic-only module request |
| `src/game/game_mode_services.hpp` (40 lines) | `da4464a976ea9fc6a838529c199a367886a18dd00c6ecdcd784e30338b99ac36` | Conforming finite composition interface; no downstream implementation certification |

**Judge contract adequacy: accept the additive qualification packet for that
purpose.** Legacy observations, input identities and evaluator are preserved.
Companion diagnostics and direct contracts close the previously identified gaps.
No blocking standards, spec or fidelity defect was found within this scope.

**Integration acceptance: pending Root.** This review does not accept the packet
as an integrated green revision, whole Mode/Session conformance, product closure,
journey closure or campaign completion. Current Main's recorded gate at base
353d528 predates these two new contracts. No new gate was run by this reviewer.

## Whole-body standards and ownership assessment

Read both complete affected files, the complete shared dispatcher, state and seam
headers, the reference adapter, and relevant Session declarations/bindings. Applied
STYLE sections 1–10 and all ten section 12 criteria, the complete-refactor brief,
JUDGE's separate-change policy and the prior review's exact qualification gaps.

The backend performs ordinary typed reads/stores against the invocation's borrowed
state. It introduces no source-global owner, cached state, fixture availability,
guest register/stack transport, address dispatch or executable access. Sequence
absence is actual game absence, checked where its signed level is consumed. State
accessors publish stores immediately; forwarding wrappers do no subsequent work
after a failed callback. Helpers and seams are named/annotated in the internal
declarations. Source entries remain single-owned; annotations are not duplicated
on their adapter definitions. No newly invented game decisions were found.

NativeServices has 27 concrete named operations and a virtual destructor. It is
explicitly NOTSA composition, owns no game state and accepts no generic dispatch
key, address or fixture-knownness mask. Its real consumer is Session's private
implementation; Session::advance passes its own flow_ and services to the shared
dispatcher. State/phase access is invocation-local, so nested native worlds do not
depend on ambient mutable routing. The existing interface's breadth is justified
by this finite composition boundary. A speculative registry, hierarchy expansion
or generic framework would not improve it. No abstraction migration is required.

All remaining uncomposed functions in the complete backend throw their named
UnrecoveredGameFlowCall, including action/adventure/mission, sequence, summary,
tutorial and staff paths. They remain unrecovered behavior, even when the file's
implementation of its seam is conforming. Some declarations are NOTSA compositions
whose recovered entry is owned elsewhere; a missing native binding is still an
explicit stop. The only successful empty behavior, request_file_module, has direct
source evidence described below. No existing behavior moved to guest/HLE.

The two files contain no FP expressions, tuning tables, imported content or
platform replacement implementation. Their narrow integer widths and signed
returns are readable and appropriate. Existing service injection/logical phases
provide composition and observation seams; this is not certification of a public
enhancement API. Formatting and function sizes in these two bodies are readable;
no whole-body STYLE12 defect was found. New tests are compact reference tooling,
not copied production algorithms or a reason to rewrite the unchanged backend.

## Contract and independence assessment

The native unit directly invokes every one of the 27 wrappers. Expectations name
the callee and exact operands independently, including unequal negative frame
arguments, INT32_MIN, high unsigned movie/character/destructor words, signed return
bits, live callback mutation, exact named null-service stops and propagated host
exceptions. It checks exactly one invocation and the pre-callback state snapshot.
Void operations are judged by effects, identity and arguments, not by fabricated
original return values.

Nine dispatcher scenarios provide 57 callback failure positions. Every positive
callback records 16 state observations, including borrowed sequence presence and
value; failing runs must reproduce the event prefix and stop with the failing
callback's pre-call snapshot. These expected prefix snapshots come from the
successful native run: they discriminate failure placement and later stores, but
are not an independent original-machine specification of every successful store.
Independent positive assertions pin the important transitions, readiness
short circuit, seven destructor kinds, callback-modified phase/mode, wrapping
increment and SuperSonic store between vsync and sound loading. The unchanged
original judge provides the separate instruction comparison. No downstream service
algorithm is copied into the unit or certified by its recording Services object.

The reference unit has 26 retained scenarios: success, 18 owner status/prefix
combinations, missing invocation, missing required return, known-zero argument,
missing movie/mode facts, a callback clearing a later fact, and nested host-error
unwinding. The complete result tuple includes failure, source/nested boundaries,
completion, pending site/callee and three optional arguments. Some prefix expected
calls are obtained from a successful adapter trace; independent literal checks of
the missing-owner/high-word and zero-versus-absent cases prevent that shared trace
from being the only payload oracle. Read the actual BindScope restoration and
lazy require_fact paths: they match those observations. The adapter contains
diagnostic metadata and marshalling, not a second mode algorithm.

The probe preserves the original C symbol, signature, 48-input validation and
256-word minimum capacity. Its companion has a separate C symbol, requires 264
words, and invokes the same run_fixture once. Both clear the caller's supplied
capacity as before. Appended words are nested boundary, pending presence,
site/callee, argument availability mask and three raw values. Present zero remains
distinct from absence; mask handling does not test the integer payload for truth.
No game branch, fixture input, expected legacy output or evaluator comparison was
changed. Invalid-pointer/count/capacity guards were read, not executed here.

The companion checker derives expected pending arguments from the frozen legacy
trace and the unchanged evaluator's ARGC table. This is an adapter contract check,
not new original-machine output. The old immutable stream anchors that trace;
the separately scripted direct unit covers missing owners/returns and statuses
that the existing fixture does not emit. The companion is not a universal proof
of every diagnostic combination or an exhaustive new original oracle.

## Independently checked retained evidence

The accompanying `mode-forwarding-integrity.json` records the final successful
read-only audit and current source hashes, including Session identities. Its checks
were implemented separately in this review; candidate oracle/mutation/build
programs and dynamic libraries were not executed.

* All 12 source-freeze baseline anchors match base Git blobs. The frozen probe is
  intentionally the old probe, not the current companion version. All source
  members, original unit and evaluator remain equal to base and this checkout.
  `git diff 353d528 0806b90 -- src` is empty.
* All nine QUALIFICATION file hashes and five retained log hashes match. Its hash
  is `7b17411a550275bf9f4d0523c6f8e1d669d39a1089796d22f1033c3a2003480c`,
  also matching the candidate manifest's evidence binding.
* Checked every one of 958 candidate manifest hashes against immutable blobs or
  the 53 retained ignored corpora. All 955 inherited paths remain; only Makefile,
  unit registration, probe and tracked mode report change. Three new protected
  oracle helpers are added. Registration and Make changes are additive; the
  changed observer/report are explicitly handled by the separate judge commit.
* Parsed all 24,000 legacy records (1,220 bytes each) and all 24,000 companion
  records (1,248 bytes each). Every legacy return code is 1, every input word and
  all 256 old output words are equal. Recomputed both full-stream hashes and input
  hash; matched both retained original `.cases.bin` files exactly. Streams hash to
  `7c28206234975f1c75e75e7737c5b6313e2796619b74a75bb4549feb15cb4f71` and
  `cc63938b3f1f9c3aa1afdb6dc7e0ec058553d30a3171de16dcaf5a5cfa370b69`.
  Input SHA-256 is
  `05c8ac3db56f68dcd6c7cb7334699f164bf2803b602a6694ec4dd7077f6d2ea0`.
* Independently recomputed all eight appended words from the retained records and
  literal ARGC declarations, every outcome-ID list and 252 distinct call-site
  paths. All 242 injected owner-failure IDs and 14 missing-sequence IDs match the
  baseline individually. These are expected fixture stops, not 256 mismatches.
* Baseline/current raw original reports both retain 24,000 cases / 252 paths /
  zero mismatches. Current report source bindings, raw report summary hash and
  baseline/current library hashes match the claimed immutable inputs/artifacts.
  These are verified retained runs, not freshly repeated original executions.

All ten FINAL faults are single, unique substitutions of immutable base source.
Reconstructed each copied source and matched its SHA-256 and actual retained file;
matched each copied contract body to the current qualified unit. Every retained
compile log is empty, executable exists, receipt says compile 0 / run 1, and run
logs identify the appropriate assertion. Swapped operands, lost movie identity,
wrong cleanup owner, changed destructor kind, swallowed callback exceptions,
reordered destructors, early SuperSonic store, cached mode, lost nested boundary
and lost pending payload are semantically valid changes. The payload mutant
survives trace-derived prefix comparisons but is caught by the independent
missing-owner payload assertion; this is useful discrimination, not a compile
failure counted as a kill. Initial unused-movie compile failure is retained and
not counted. No mutant was rebuilt or executed in this review.

## Direct no-op evidence and precision limits

Inspected the retained complete body, disassembly, raw profile-bound receipt and
checker. The 21 bytes
`8b4424045068c0de7d00e891d40f0083c40833c0c3` load the first argument,
push it and the format pointer, call the printer, restore eight stack bytes,
clear EAX and return. Recomputed body hash
`f9c3a61ebc7bc8f77c7804fec4fdf357b2f58602c3138e49dda0c67d58186fa4`,
decoded relative call target and complete instruction-byte concatenation. The
printer receipt is one byte `c3`. The 25 retained executions cover five distinct
words in each caller argument, preserve argument storage/stack discipline and
return zero; the original checker executes the actual body without callee hooks.
The second argument is unused. The dispatcher ignores EAX. This supports the
native no-load/no-state-change implementation; it does not qualify general
diagnostics, resource loading or a future profile.

The current immutable patch census hashes to
`926395f83d0c5d92455ef93df91c950718057ae77b99622fb8e1e19cbf0f0e1e`.
Independently recomputed overlap tests for both instruction ranges: no entries.
This closes the prior direct-body evidence gap within the static pinned profile.
No source executable is present in the review checkout (nor at the candidate's
standard asset locations); an attempted read stopped before opening anything.
Thus the PE bytes/source hash are retained producer evidence, not a fresh PEImage
read by this reviewer. No original execution or live mod execution was performed.

The dispatcher judge substitutes owner calls and has no downstream FP arithmetic
or FP-status qualification. Full integer observation equality does not turn
inherited cleanup/title FP partials or failed downstream cases into passes. No
new holdout ran; the historical 2,048-case holdout and three historical mutants
remain the prior review's bounded evidence, not new current full-tuple holdouts.

## Actionable integration priorities

1. Integrate the exact reviewed judge/observer/contracts additively, preserving
   every parent protected path and its reviewed identity. Reconcile with the
   independently pending save/checker packet rather than blanket-refreshing the
   edited checkout. This candidate's 958-entry guard does not protect the two new
   unit bodies; their exact current hashes are frozen in QUALIFICATION and in the
   mutation receipts. The future widened guard must include those same hashes:
   forwarding `e97aea5ea68cfb5bc29b0b4cc81c416b5123eab547770b015638ea1a99e233b5`,
   diagnostics `603922c6bc3e688960f0cdce648ecbeed8cda82aee98252d3356207a807a00d4`.
   Future 955/958/1259 parent union and widened protection are **not done here**.
2. On the actual integration revision, execute the registered contracts and
   required canonical checks, verify immutable current source/probe/report
   bindings and the final manifest union, then obtain a coherent gate with its
   actual exit status. This packet is not already integration-qualified by Main's
   older 292-native/450-Python gate.
3. If Root needs product or broader Session disposition, bind actual current
   closure/journey receipts and downstream implementation reviews to their
   current source dependencies. Test Services does not certify Session's private
   title-close argument handling, resource lifetime, audio, save or other service
   algorithms. No two-file approval authorizes inferring those properties.

The early source-freeze note about unresolved direct no-op evidence describes the
before-edit checkpoint; the later profile-bound receipt closes it as above. Keep
those chronological identities rather than rewriting baseline history. Initial
test enum/setup failures and rejected 19-byte extraction are documented failures,
not successful qualification runs.

Only this detailed report and its read-only integrity report are authored. No
STATUS, HANDOFF, global inventory, maintained source, judge, candidate, Main or
global memory edits; no build, oracle, mutation, gate, reserved holdout, delegation,
provider/download/network/storage/host change or push. Review-script setup checks
initially treated the deliberately frozen old probe as current and guessed an
absent unbound raw receipt filename; corrected final checks above passed. They
were reviewer setup errors, not candidate failures or rerun qualifications.
