# Gamma display draw packet — incomplete

Base: `fbc1eec`, branch: `packet/draw-path`. This packet does **not** complete
E102Display or submit an animated Gamma body. The actual constructor/display test
still reaches `native_gamma_display_action_clip_no_scale`. No N3b or visible actor
progress is claimed.

## Implemented and checked

Source commit `7ad3c08` recovers `__DrawModel` in the existing object-draw module
and shares its dispatch with static traversal. It uses the ordinary maintained
model renderer. Valid imported handles replace source pointers; zero is null,
and low invalid guest-pointer classification belongs to reference bindings.
No arithmetic, material owner, geometry owner or Gamma display boundary changes.
The existing object-draw test and actual Gamma constructor/display caller pass.

The judge addition preserves all 1,126 topology cases and their observation JSON
semantics. Native output reconstructs the retained passing original commitment
exactly: `85f83fbbc0beb9cfd263530fa7003da887089358ac1800dd7105991972820c9d`.
`replay_retained.py` makes this check reproducible without claiming a fresh source
execution. The old 1,066 render-state pass is retained evidence, not freshly rerun.

`--body` adds a separate placement cohort. Original E102Display instructions and
integer render leaves, including SetMaterial's scratch stores, execute up to
`0x4800b4`. Palette implementation and Ninja numeric leaves are explicit common
seams. It compares collision stores, matrix-operation arguments/order, render
scratch, callback identity, animation ordinal, and event versus ordinary decision.
It stops **before** the later frame load, mode recheck and table/generated action
selection. It does not test palette backend behavior, matrix numeric parity,
animated traversal, mesh submission, pixels or FP exception flags.

Five isolated compiled faults are proposed in `faults.json`. All compile and
change native output across 1,126 topology plus 1,024 body cases. These are native
sensitivity results, not original-body mutant kills. Native-only differences are
793, 1,241, 1,177, 963 and 654 respectively. Body development and the separate
holdout still require original execution. The holdout command uses 4,096 random
cases and seed `0x9e47fd50`; it was not run.

## Source recovered for continuation

`SOURCE-EVIDENCE.json` and `evidence/` preserve source-bound dossiers, instruction
extracts, evidence IDs, body extents, hashes and profile intersections.

* `0x4800de` is E102Display's call site; ds_ActionClipNoScale begins at `0x407020`.
  The wrapper supplies frame, flags zero, clip scale zero and `__DrawModel` to
  DrawAction, with the action passed in EAX.
* DrawAction first checks loop_count, action/root/motion pointer validity and
  texture readiness. It publishes the model callback and flags, clears clip
  scale, initializes motion, selects transform functions, traverses the root,
  then clears clip scale again. Guarded early returns publish none of those stores.
* Animated traversal always pushes before transforming, distinguishes object flag
  64 from sampled transforms, invokes its own callback slot, reloads the model,
  draws it unless hidden, visits the child, pops, then reloads the sibling. It
  does not apply static traversal's Break flag.
* The MDATA2 transform target `0x404fd0` was absent from Ghidra's function table.
  Its decoded instructions are preserved individually with evidence IDs: pointer
  and count bases differ by eight bytes, each node resets the channel index,
  translation/rotation/scale run in order, and the data cursor advances 16 bytes.
* `njSetObjectCallback` owns `0x3d08844`; D3D animated traversal reads `0x3ab9908`.
  Their xrefs are retained. Do not invent a bridge between these distinct slots.
  The recovered Gamma callback compares object rows 12 and 28, using E102WK aim
  rotation/translation when reached. It is not implemented by this packet.
* Angle sampler `0x404c10` has the active loader hook `LinearMotionA_r`.
  `InterpolationFixes.cpp` initializes its enable flag false and exposes push/pop
  via HelperFunctions. Original sampling is selected only while that flag is
  false. Its actual normal-input state was not measured here. The new model
  callback itself has no active profile overlap.

## Branch and frontiers

The fresh actual constructor/display test reaches the **ordinary empty-event-list
branch** and passes its event and missing-asset checks. The user-supplied normal
app observation at frame 1332 / action_clip_no_scale also implies that branch,
but this packet could not reproduce that normal-input observation.

The freshly rebuilt app exits 1 before its first game frame with
`native session stopped: Metal device unavailable`. It creates no screenshot;
frame and screenshot hash are null in VERIFICATION.json. No retained image is
presented as a new capture.

The ordinary draw remains the existing explicit stop. Event animation, walking
sway, hidden tail, generated/ordinary action rendering, subsequent arm action
export 50, attachments and later display tail remain unresolved. No source
behavior was skipped to move the frontier.

## Execution limits and admission

Unicorn 2.1.4 traps during initial mem_map, before emulation. This sandbox denies
`sysctlbyname("hw.cachelinesize")` with EPERM; Unicorn falls back to CTR_EL0.
The original-body development attempt exits 132. No library patch, hardware
value guess, tolerance change or verifier bypass was used.

`make -j2 check` reaches judge registration and exits 2. Layers, global ownership,
provenance, readability and constexpr checks pass. The protected manifest has
unregistered/changed inputs inherited from the base plus this judge proposal;
registration belongs to the orchestrator. Full gate, full oracles, source-only
build, fresh render-state oracle, differential body/fault/holdout verdicts and
normal-input screenshots were not obtained.

All builds use `-j2`. Assets are cloned/imported only under build. Both owned
Ghidra sessions received close; REA reported cleanup confirmation failures because
process inspection is denied. Their logs subsequently show completed post-analysis
and discarded imports, empty project directories and removed bridge sockets.
No Ghidra session remains intentionally open.

A transient index.lock denial initially affected Git operations; later git add
and source commit succeeded. This is not a remaining Git blocker. No push occurs.
An independent reviewer found and corrected the new texture hook's cdecl stack
cleanup and clarified the pre-action-selection scope; remaining review limits
are recorded in VERIFICATION.json.

The next worker needs an execution context with Metal and working Unicorn before
qualifying and completing DrawAction's animation runtime and its actual caller.
Do not admit this packet as a completed Gamma draw path.
