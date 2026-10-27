# Independent Gamma ordinary texture composition review

**Bounded approval: no new blocking STYLE or fidelity findings.** First review of this
texture composition, frozen candidate `d7e912ae7824af3ed2fb2caae4eef74384b570b1`,
base `6e9a1ec9aafa255591ede7c9ca7695b54e2a377a`. Reviewed the candidate worktree
`/Users/burbujamc/.t3/worktrees/sadx-macos/t3-b75da23e` read-only. Reviewer branch is
`packet/review-gamma-texture-binding`, based on Root Main `38cfd34`, in its own worktree.
Only this review directory is authored. No Main or candidate writes and no push.

**Tests NOT RERUN.** No builds, tests, oracles, mutants, apps, gate, Ghidra, providers or
subagents were run. Source, retained evidence, logs and images were read; file hashes and
Git object identities were compared. AGENTS, GOAL, STYLE including §12, STATUS top,
HANDOFF and JUDGE were read. The code-review skill's Standards and Spec axes were applied
within the user's explicit prohibition on further delegation. Recorded execution and
holdout chronology remain author receipts, not independent execution by this reviewer.

## Standards

Zero new findings. `CharacterTextureViews` (`src/runtime/character_texture_assets.hpp:18`)
owns only invocation-local descriptors. It borrows Session's archive string storage and
the actual List objects; `catalog()` borrows the existing signed-byte array. Its noncopyable
view and local empty sentinel keep internal pointers stable. All nine known-present source
tables remain nonnull, including present-empty tables with count zero. The count bounds
the imported descriptors and represents the omitted source sentinel, under the documented
capacity/termination contract. No shadow texlist, counter, texture manager, GLOBAL owner,
Session attachment or Services hierarchy is added.

`TitleScene::exec_selection_gamma` (`src/runtime/native_selection_preview.cpp:189`) keeps
the view and existing regular Services alive across synchronous `gamma_selection::run`.
The actual Session State is borrowed. Neither Context nor its callbacks are retained by
the new loader; the existing scheduler callbacks are function identities, not captures of
the local view or services. Existing Session list-vector/string storage remains stable
through the call; lower loaders mutate list contents, not their enclosing owners.

The binding (`src/runtime/native_gamma_binding.cpp:206`) directly invokes the canonical
ordinary wrapper and fails loudly if its State/Services are unavailable. It adds no
counter arithmetic, warm assumption or selection-wrapper substitution. The actual native
Amy definition throws `native_LoadEggRoboTextures`; its sole canonical STUB annotation
remains in the game header. The separate one-comment commit `0e99102` removes Gamma's
obsolete duplicate `421280` STUB, satisfying the prior review's integration obligation;
the complete FUNCTION belongs to `character_textures.hpp`. Gamma's algorithm is unchanged.

STYLE §12: new code uses semantic names, structured control flow, direct calls and NOTSA
adaptation notes; content comes from the imported assets. No PE reads, address dispatch,
reference-tier relocation or new arithmetic abstraction appears. The wrapper's integer
byte increment preserves modulo 256 behavior; this composition adds no FP calculation.
The saved static-check receipt reports no regressions; no checker was rerun here. Existing
resource seams support alternate delivery without editing the algorithm. No new platform
replacement or speculative enhancement framework is introduced. The inherited component
CPU Context alias is outside this diff; no existing arithmetic debt is claimed removed.
The prior collision review's two comment nits are already fixed in Main and are not new
behavior findings against this preserved candidate base.

## Spec and composed behavior

Zero new findings within the requested boundary. Approved ordinary core `8052fac`, reviewed
at `7ad2c4b`, is imported byte-for-byte in all six source/unit/probe/oracle/mutant files.
Complete wrapper source evidence is
`ev_49279b700b5fec055393ce16c7aa84e0ef6d683b686745a628bcc0a54435447b`.
Its captured-table traversal, next-descriptor read after each lower callback, live counter
increment, exact-zero cold test, named/nameless distinction and cold-only Amy extra remain
intact. This is ordinary `421280`, not selection `512550`. The prior approved collision
branch still resolves exactly to `6e9a1ec`.

The new composed consumer uses all five actual imported Gamma archives and Display's real
texture manager. It checks counter/catalog/list/archive-text pointer identities, source
archive order and populated lists, cold/warm calls, negative-to-zero behavior, actual reload
after wrap and cached Memory identity. Present-empty handling and the actual native Amy
throw are exercised separately; the latter must stop before increment. Existing player,
current-state, collision and scheduler cleanup checks remain. The cold test is distinct
from the actual app's already-warm selection path; neither is substituted for the other.

The app's diagnostic reads the canonical counter array by const reference and serializes
signed values. It introduces no state injection, shadow counter, game branch, clock or
pacing change. Retained normal-input command/exit/log receipts show frame 1382, diagnostic
exit 1 at `native_gamma_weld_binding`. Capture 1382 has Gamma count 1 before the callback;
the terminal report has 2, with all other bytes unchanged and 762 resident slots in both.
The saved image was inspected: it is outgoing title output, byte-identical to the prior
collision stop. This is progress through an initialization boundary, not actor pixels.

Weld, cursors, effects, materials, animated matrices, draw and later seams remain loud.
No new weld/effects/cursor/material/matrix/render algorithm is supplied or approved.

## Evidence and judge integrity

`audit.py` performs only retained-file/Git reads and hash comparisons. Its `evidence.json`
records **83 matching checks**, including all 15 proof source hashes, six approved-wrapper
Git identities, unchanged Gamma and regular algorithms/harnesses, raw report/log/image
hashes, the real command/exit/counter transition and exact prior-branch preservation.
The old judge manifest, readability baseline, Makefile and unit manifest are unchanged.
Root's old exact-pack correction `9fa9966` is not imported or silently refreshed here.
The existing composed consumer is deliberately extended to the new boundary; no original
Gamma/startup/regular expectations, tolerances, fixtures or mutants are altered.

All six retained composition-mutant source hashes and assertion logs match their summary.
Build exits are recorded 0 and test exits 1: omitted wrapper and wrong character fail cold
load checks; shadow count and discarded texlist fail pointer identity checks; absent-empty
fails nonnull-table identity; silent Amy fails the throw-before-store check. These are
assertion failures, not compile errors counted as kills. All ten wrapper mutant sources
match their hashes and their reports contain actual value mismatches with the stated
first case. Their scope remains upper-wrapper behavior, not lower algorithm proof.

Recorded focused binding unit, frozen wrapper/Gamma/startup/regular units and force-load
closure exit 0. Wrapper original reports are strict passes: 5,297 development and 4,096
holdout cases with zero mismatches. Downstream regular reports 609 cases, zero mismatches.
Gamma reports remain partial: 1,422/0 values/498 FP and 1,080/0/690 FP, both rc 2.
Startup remains 2,004/0 mismatches/924 FP partials despite the old script's rc 0.
None of these partials is promoted to a strict pass or lower-callee completion.

## Root handoff and limits

Root retains source/test/probe/judge registration, current-Main consumer rebuild, source-only
export, merge and combined full gate. The private stale manifest is intentionally not a
green-gate claim. Main's last full gate is `afc3e94`, rc 0, at the earlier CCL stop;
the later collision and texture focused receipts do not replace it. This review grants
no full-gate approval and no actor/GPU/N3b/story/gameplay/controller/63 FPS qualification.
The complete native story route remains open. No shared memory note is authored because
the user restricted writes to this review directory; this committed record is durable.

Standards: zero new findings. Spec: zero new findings within this bounded composition.
