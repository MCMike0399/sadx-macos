# Native CharacterSelect display checker — Worker B

Base: `fbc1eec94284e848170f99d94202ec328a59f208`, branch `packet/cs-journey`.
This packet adds testing tools only. No shipping draw hook, game behavior change,
Worker A owner edit, gate registration or N3b completion is included.

`private_native_assets.py` creates a previously absent output directory using
independent APFS clones (ordinary copies when cloning is unavailable). It rejects
symlinks/special files in the source, refuses overlapping source/output trees and
all existing outputs, and checks the pinned canonical selection hash before and
after copying. Appended pack admission checks every old byte except the count,
record counts, full pack digest, prefix digest, round-trip declaration and Gamma's
eight display declarations against gamma-actor metadata. The private prefix is
reconstructed with the canonical count and hashed again. This checks append
consistency; it does not recover or qualify the animated renderer.

The normal-input checker copies and hashes all three saves from the November 2
receipt into each new run. It uses the existing five Enter presses, offscreen
paced execution and a final screenshot (also emitted by the app at a caught stop).
It checks mode 12/state 21, activated SonicDX02 slot 1, no fallback, canonical
Gamma registration, scheduled paths/exec/display flags, live preview/camera and
initialized Gamma mode 1/ID 2/player index 5. The FileSelect selected-name observer
is correctly null after leaving that screen. These expectations come from the
retained orchestrator log in SCHEDULER-OBSERVATION.json, not invented save state.

Default drawn-goal mode requires a complete nonuniform 640x480 payload, rejects
the known outgoing title hash and compares against the existing committed
reference R3 CharacterSelect coarse signature through capture_content.py. It
also requires the requested frame and explicit application exit. Boundary mode
still evaluates and records that goal assertion, but separately checks the exact
named stop/frame/exit and pinned outgoing title bytes. A boundary-mode success is
not a drawn-frame pass. Neither mode is registered in the gate.

## Commands

Run from this worktree. Every output below must be new; substitute a new name on
repeat. The verified save and appended pack inputs are read-only.

```sh
python3 tools/testing/private_native_assets.py \
  --source /Users/burbujamc/Developer/sadx-macos/game/imported \
  --output build/cs-private-assets \
  --append /Users/burbujamc/.local/state/sadx-macintosh-reception.oUTxv5/extracted/gamma-display/build/gamma-display-native/assets/characters/sonic-selection.actions
make -j2 -o build/native-controller-deps/include/SDL3/SDL.h build/sadx-native
python3 tools/testing/check_native_character_select_display.py \
  --assets build/cs-private-assets \
  --save-source /Users/burbujamc/.local/state/sadx-macintosh-reception.oUTxv5/extracted/gamma-display/build/gamma-display-native/baseline-save \
  --output build/cs-observe \
  --expected-boundary native_gamma_display_action_clip_no_scale \
  --expected-frame 1332 --expected-exit 1
```

The Make wrapper is `make -j2 native-character-select-display` with
`CS_DISPLAY_ASSETS`, `CS_DISPLAY_OUTPUT` and `CS_DISPLAY_ARGS` (save source and
optional boundary expectations). `make -j2 private-native-assets` accepts
`NATIVE_ASSETS`, `CS_DISPLAY_ASSETS` and `CS_ASSET_ARGS='--append PATH'`.

The deliberate break inserts `continue` immediately before the renderer consumes
each graphics::Draw, preserving clear commands. It compiles that private renderer
and links a separate executable, excluding the shipping renderer object. The
maintained source is never patched. Exact fault command:

```sh
python3 tools/testing/character_select_draw_fault.py \
  --output build/cs-draw-fault-final -- \
  --assets build/cs-private-assets \
  --save-source /Users/burbujamc/.local/state/sadx-macintosh-reception.oUTxv5/extracted/gamma-display/build/gamma-display-native/baseline-save \
  --expected-boundary native_gamma_display_action_clip_no_scale --expected-exit 1
```

Fault runner exit 0 means a compiled mutant reached the correct semantic state
and the checker rejected its actual uniform capture. Exit 2 means unqualified;
a compiler/startup/Metal failure cannot count as a caught image fault.

## Observations and limits

Fresh asset tooling passes: zero links; canonical hash c414e173… before/after and
in reconstructed private prefix; private appended pack 382e3e53…. Fourteen fixture
tests pass through Python and the registered C++ unit runner. They cover private
copy independence, links/overwrite/overlap/hash refusal, corrupt prefix/metadata,
blank/wrong nonblank images and wrong save/state/scheduler flags. Existing
native-gamma-display unit program compiles; it was not executed or qualified here.

Fresh application attempts stop at `Metal device unavailable`, exit 1, before any
state/frame/screenshot. Both boundary mode and default goal fail honestly. The
private mutant compiles, then has the same startup failure; runner exit 2,
qualified_image_rejection=false. Thus actual rendered suppression remains OPEN.

Reused orchestrator evidence reaches frame 1332 and stops at
native_gamma_display_action_clip_no_scale (application exit 1 per worker brief).
Its image hash is c7d02f642ae8e5493f19051e7cf13a06a2d1d103351e72b0576a108b52aba427.
Its semantic state passes this checker; its image fails the drawn-goal assertion.
The retained PPM was inspected and is the outgoing title, with no actor frame.
These are reused observations, not a fresh successful Worker B runtime run.

`make -j2 check` exits 2 at judge-manifest validation, 29 violations: 27 already
exist in base fbc1eec, plus the two new test sources. All preceding static checks
pass. Per assignment, no judge manifest, gate tool or old expected value changed.
Orchestrator must reconcile/register protected inputs; this packet cannot claim
a green check. Unrelated worktree hydration/symlink changes are left unstaged.

Before the drawn-frame flip: run this packet in a Metal-capable context, reproduce
the exact boundary, run the compiled suppression fault and retain image rejection.
When Worker A's draw lands, run default mode at the intended capture frame and
qualify reference alignment. The existing R3 signature is from frame 1700 with
Sonic, not an aligned first-Gamma pose; it is a screen-level initial requirement,
not evidence of Gamma pose equivalence. Any actor-specific aligned reference
requirement needs independent source/reference evidence, never a native blessing.
Only then register the drawn journey. Full gate, physical controller, audio parity,
Windows parity, all actors, cursor/cancel/reentry, source-only rebuild and N3b
acceptance are outside this packet's new qualification.
