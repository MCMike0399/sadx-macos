# Independent shared depth-state review

Decision: **APPROVE the bounded depth decision refactor and corrected private
depth observer.** No new actionable correctness or design finding. Both P2
observation gaps from review `3cbf412` are closed. This does not approve the whole
render module, complete owner/consumer migration, campaign completion, or final
integration qualification.

Reviewed `72f2822acc27889db0ac96813801bf7b6a988da8` on
`packet/render-state-refactor`, worktree
`/Users/burbujamc/Developer/sadx-wt-render-state-refactor`, against approved blend
`7fe9f364e852470c50dd350d091154904a7685f3`. Source scope is the new
`ninja_depth_internal.hpp`, depth methods in `ninja_render.cpp` and
`native_display.cpp`, and `native_render_state.cpp` with its unit registration.
Separately reviewed the depth judge imported by `b932869` (including correction
`b40093c`), discovery wrappers and explicit probe rule in `e4ec806`.

During inspection the candidate advanced to `d480e897109b3060307022ee9c62368b20edcd73`.
Verified its diff from the requested revision contains no changes to `src/`,
`tests/`, `tools/oracles/` or Makefile. The later manifest/inventory/registration
receipt is outside this review's certification; the decision remains pinned to
the requested source and judge scope.

## Findings and qualification limits

There are zero new defect findings in either the standards or specification
assessment. The following remain explicit qualification work, not newly found
regressions:

1. The legacy compatibility facade still carries fixture optionals, source PCs,
   address-shaped fields and diagnostics in engine files. Those files do not yet
   satisfy whole-module STYLE sections 1, 2, 6 and 12.1. The semantic core is shared
   by actual native Display and the legacy adapter; no behavior was relocated to
   guest/HLE or excluded from maintained builds to hide this debt.
2. The final 1,024-case holdout was not run. Root owns final candidate holdout,
   manifest/inventory reconciliation, affected consumer regressions, updated
   clean native build/closure and journeys, and the coherent integration gate.
   Main's green enforcement gate and the earlier clean blend build do not certify
   the larger depth packet. No final depth/journey claim follows from this review.
3. These leaves and the observer qualify command production and explicit prefix
   stops. They do not qualify GPU rendering, Windows COM implementation, actual
   hardware fault completion, texture-stage policy, every issue-helper consumer,
   arbitrary callback reentrancy, or the complete render-state system.

## Source fidelity and STYLE section 12

- `ninja_depth_internal.hpp:10` sequences device acquisition, masked comparison
  lookup and issue in separate statements. The mask is unsigned 7; the table
  returns the original full word. The device is retained by value. There is no
  eager comparison-table access while constructing the adapter lambdas.
- `ninja_render.cpp:131` preserves unknown-device, known-null and missing-table
  precedence and exact load/dereference/table boundaries. Its local exception
  catches only adapter read stops; unrelated renderer exceptions still propagate.
  Removing the old redundant device validation adds no intervening callback or
  fixture hook: acquisition and table access cannot externally mutate the device.
- The comparison path calls the unchanged `issue_on_device` with the acquired
  handle. It copies that handle into the published command before invoking the
  backend. Replacing/clearing the live runtime device during a callback cannot
  invalidate that command or its pending copy. There is exactly one issue and no
  subsequent game read or write requiring device reacquisition in either leaf.
- Command publication precedes the missing-backend check; incomplete completion
  retains the attempted command, source call boundary and full pending payload.
  Completed HRESULTs, including negative ones, are appended and ignored for game
  success. The rewrite neither skips missing output nor masks a failed prefix.
- `ninja_depth_internal.hpp:19` accepts and issues a full `uint32_t`; 42 remains
  42. The legacy write path still uses its existing live-device issue owner and
  boundaries. Native Display keeps its existing bool interface, producing 0/1
  without narrowing the legacy contract. State IDs remain comparison 23/write 14.
- Both native methods update the existing `device_state_` owner; neither creates
  a replacement cache or reference-tier dependency. The added include follows
  runtime-to-engine layering. Draw snapshots still copy state through the actual
  public renderer. No new friend/testing accessor was added.
- The shared header uses semantic names, structured control flow, scalar literals
  and narrowly justified `NOTSA` helpers. Callback parameters express the lazy
  reference reads while allowing the native owner to use the same decision. This
  is a small internal seam, not a new service framework or ownership abstraction.
  Provenance of the decisions was checked against literal original instructions.
- Arithmetic is plain integer mask/pass-through; no numerical FP algorithm,
  control word, tolerance or exceptional-value behavior changed. No new game
  callee, fallback value, STUB substitution, enhancement behavior or platform
  backend was invented. Remaining address/provenance/platform annotation debt in
  the existing facade is not certified away by extracting these decisions.
- Existing blend implementation, interfaces, judges, probes, fixtures, tolerances
  and mutation specifications were not changed by the depth source delta. Their
  earlier approvals are not replaced by this review or the new depth judge.

## Independent observer and registration

The corrected probe's words 11–15 record callback count and actual delivered
device/state/value/call-site fields from its own callback capture vector. They
are independent of returned `result.commands` words 2 and 5–8. Consequently a
correct ledger cannot hide a wrong backend value or duplicate callback. Words
16–19 serialize the actual pending command, independently of that ledger; word 4
still checks presence. This directly closes both prior P2 findings.

The source side observes actual COM stack arguments at the original CALL when
the backend is missing, and at method entry when it is present. Method entry
checks its actual return address against the verified six-byte caller CALL.
Missing backend stops before entry with callback count zero. The incomplete
external fixture enters once but supplies no completed result, stops execution
there and attributes the logical failure to that verified caller. It does not
pretend the original leaf returned normally or completed a hardware fault. No
game result or callee is substituted. Device identity normalization is explicit
and checked against the fixture's actual device address.

In-memory `gen_make.oracle_recipes()` discovers exactly one
`oracle-ninja-render-depth` target through the new `verify_original_` wrapper,
with the canonical native probe and isolated `build/oracles/ninja_render_depth.json`
output. The generated ORACLE_TARGETS aggregation includes discovered recipes.
The explicit Make rule compiles the wrapper plus `ninja_render.cpp` and lists
both private headers, public header and underlying probe as prerequisites.
The new unit source is discovered by the existing wildcard/pattern rule, and
`native-render-state` occurs exactly once in the execution manifest. No generator
or existing blend recipe/ABI was changed to register this sub-judge.

## Evidence inspected and checks actually run

Read AGENTS, STYLE including section 12, JUDGE, PROMPT-complete-refactor, GOAL,
ARCHITECTURE, STATUS top, latest HANDOFF, render-state-packet context, prior review
`3cbf412`, approved blend review and the depth judge/shared evidence directories.
Inspected the actual native draw composition test and renderer call path. Its
assertions exercise all eight comparison values, modes 8/42/UINT32_MAX, write
false/true and retained earlier snapshots. The earlier `first` reference is not
used after another draw can reallocate the command vector; the retained snapshot
is reacquired by index. This review inspected its recorded exit 0 and source,
but did not rerun or rebuild that native composition executable.

Independently read and disassembled the two original ranges using PEImage and
Capstone, with pinned PE SHA-256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
The bytes establish device load/vtable dereference before comparison table read,
mask 7, full-width write, method offset 200, state IDs 23/14 and CALLs at
0x77ed19/0x77ed2f. The literal comparison table is eight distinct words 1–8.
Verified both range hashes against the committed reports. No provider was used.

Ran a read-only, in-memory development replay with Unicorn 2.1.4 and the existing
shared-depth dylib after verifying its SHA-256 and source/probe/checker/header
fingerprints against development.json and proof.json. **1,998 cases, zero
mismatches across all 20 words**, exactly matching both committed current rows
and the corrected pre-rewrite rows. No report or bytecode cache was written.

| Leaf | Returned | Unknown device | Known null | Missing table | Backend boundary |
| --- | ---: | ---: | ---: | ---: | ---: |
| Compare | 880 | 42 | 42 | 21 | 14 |
| Write | 887 | 42 | 42 | 0 | 28 |

Verified corpus hashes for the original setup, corrected setup and shared
development receipts. Original source fingerprints match unchanged `1a903ce`;
all 1,998 input identities and first 11 meaningful source/native output words
are preserved. The old twelfth word was unused zero; the correction uses it for
the independent callback count and extends the observer to 20 words. Original
setup receipts were preserved separately.

Inspected all six existing current mutation reports and their 1,998-row corpora,
including corpus hashes and mismatch counts: wrong mask 447, boolean write 897,
eager table 42, wrong callback value 1,781, duplicate callback 1,788 and corrupt
pending 38. Receipts report successful compilation and failing judge exits.
For mutated cpp sources, checked detailed report fingerprints against the exact
in-memory transformed source; header hashes are supplied by the packet proof,
not the checker's four-file source list. Current restored source hashes match
that proof. No source mutation or mutant execution was performed by this review.

No build, full gate, holdout, game/app run, source fix, provider, other agent, SSH,
storage helper, HANDOFF, STATUS or Main edit was performed. The reviewer changed
only this report and PACKET.md in its assigned durable review packet.
