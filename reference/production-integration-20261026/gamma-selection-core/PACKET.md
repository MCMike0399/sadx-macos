# Gamma selection core packet

Branch: `packet/gamma-selection-core` in the assigned durable worktree.
This packet recovers E102 mode 0 and its mandatory common tail. It does not
recover the other mode bodies, E102Display, E102Destruct or the lower callees.
Root owns product binding, imported resources, review and the full gate.

## Native API and borrowing contract

`sadx::gamma_selection::run(Context&, const TaskHandle&)` is the bounded entry.
`Context` is opaque. Its free bindings and lower seams are declared in
`src/player/gamma_selection_core.hpp`; there is no new services hierarchy.
The native implementation supplies those definitions and fails at every missing
resource or unrecovered lower call. The probe's receipt definitions are test-only.

Bind `CurrentView` to `{actor_current_state.primary, actor_current_state.player}`
from Root's separately reviewed plain shared owner. Never bind fixture optionals,
instantiate broad legacy Sonic globals, or create Gamma-only current slots.
Gamma makes no super-byte stores. Publications are current primary, old player,
previous hover, cleared hover, event predicate, then time-zero predicate. The
player local reloads through the captured motion after PInitialize; the current
player publication remains old.

`Primary` is the existing `player_initialization::PrimaryView`. Its references
must point to the captured Task.primary_state, including ID 2 and the existing
preview pose. Motion and player bindings use Task.motion_work/Task.motion_state,
the arena's motion-to-player reference and player_initialization::Registry.
The initializer seam calls the existing PInitialize with those owners and the
typed imported parameter table; it must not synthesize a player or reset aim.

Gamma free work is `playerwk.free.ptr[0]`, separate from PInitialize's auxiliary
allocation at `island_list`. `store_extra_work` and `extra_work` use that actual
arena reference. `ExtraView` borrows the one typed E102WK associated with the
allocation. Its fields are laser mode, general timer, ground Y/distance/attributes
and walk distance. CAlloc uses 1 × 580 bytes. On null allocation the source still
stores null and timer 0, then calls cursor initialization; it subsequently faults.
Do not turn that into successful initialization or an early skip.

`State` owns only Gamma's unique publications, matrices and line endpoint buffer.
The NJS_POINT3COL points pointer borrows `State.line_vertices.data()`. Its colors
pointer borrows the imported mutable two-color record; texture is source null,
count is 2. No action/object payload belongs in this line descriptor. Material
lookups borrow the existing shared mutable graph, including aliases between
table rows. The headlight pointer borrows the actual final material record.

All binders are per-consumption owner access: fail on missing facts, preserve
native pointer/resource identity, and do not add game behavior or hidden callbacks.
The source stores actual destructor then display callbacks before CAlloc. Both
callback getters must return those real native callback wrappers or fail loudly.

## Reached lower calls

The source entry calls EV_CheckCansel and IsTimeZero. Root must use the real
cutscene-task owner and bootstrap frame/second/minute bytes for them. An existing
player additionally reaches laser-mode update, CheckEditMode, action processing
and action-state update before dispatch. Nonzero mode bodies remain explicit
unrecovered-mode boundaries; this is not a complete controller.

Mode 0 reaches PInitialize; complete animation table/request 0; CCL_Init with
three records and ID 0; first collision extent copied from live parameter height;
LoadCharTextures(6); the weld binding; actual callbacks; CAlloc; cursor initializer;
line setup; setup-resource loader; five material edits; optional ID 1 time HUD;
and the real effects initializer. ID 2 skips only the ID 1 HUD branch.

The common tail publishes the local extra-work pointer, then reaches PSetMotion,
PJoinVertexes, laser-target update, model matrices for nodes 54/30/39, Z rotation
16384 on node 39, matrices for nodes 2/37, fan update, E102Display, post-display
effects, optional held-task update, Shadow(scale 1.5), ground attributes, optional
running effects, collision range/list, underwater behavior and LoopTaskC. Later
extra-work pointer reads remain live; the ground-work local is captured before
Shadow and retained across the ground-attribute call. No tail call is suppressed.

The two source setup callees retain neutral names. Source 49fb10 calls 4cc330
with 91d15c, 91d168 and 0, then tails to 49f9f0. Source 4c5420 reads 982148,
calls 456c50(value, 0) and stores 982154. The controller contains no direct
event 580 call. Historical notes do not override these bytes.

## Exact resource requirements

`prove_gamma_selection_sources.py` executes the complete original animation and
weld producers with sentinel declarations, and checks the pinned model DLL.
It emits full 91-row recipe metadata privately, not model/motion payloads.

* Gamma has 78 declaration slots; export 61 is genuinely null. Recipe row 72
  refers to constructed action 3c5d530, whose producer is a separate dependency.
  Preserve declaration 61 as known null, distinct from unavailable payloads.
* Request 0 uses export 0, Stand: action 1020e344, object 10207290, motion
  1020e338, 77 objects, 16 frames, channels 3, interpolation 2. Existing selection
  export 2 cannot substitute for it.
* Bind all 91 recipes as the complete table. Preserve unavailable actions as
  resource boundaries; do not fabricate idle actions or null replacement tables.
* The weld list uses object row 0 as root and source object rows 24, 25, 26, 27,
  with vertex counts 2, 2, 0, 0, modes 4, 5, 6, 7, then a full null sentinel.
* CCL_INFO is three 48-byte records starting at 91c5b8. The line colors are two
  four-byte records at 91c648. Character texture identity is 6.
  Its actual declaration has E102 → 91c560, E102BEAM → 91c568,
  E102EFFECT → 91c570, E102TIME → 91d5e0, E102HIT → 91d15c,
  then a null terminator. Reuse selection_preview's existing texture owners.
* Mutable material table row 1/material 6 and rows 5, 4, 3, 2/material 2 must be
  available in that order. Preserve aliases and the shared graph lifetime.
* Cursor initialization requires the actual WIREMODEL templates at 981ea0/981ec0
  and its reached allocator/setup calls. Effects/setup resources remain lower
  dependencies; their successful implementation is not part of this packet.

Source range hashes and dossier IDs are recorded in `source-evidence.json`.
`context.md` records the evidence bundle and semantic corrections. The profile
scan finds zero active or undetermined overlaps in the three complete ranges.

## Verification and limits

Final counts and exact commands are recorded in `proof.json` after the focused
checks finish. The registered native unit joins the real task pool, canonical
primary/motion, actual PInitialize and the receipt-bound Gamma core. The new
source oracle executes the original entry through its actual return or explicit
mode/lower/fault boundary, comparing call arguments and publication/state prefixes.
It covers allocation failure, material faults, old/new player publication,
primary ID/index mutation, aliased material rows, extra-work replacement and
retained Shadow returns. Seven mutants and a separate holdout qualify that slice.

The retained Shadow return uses ExactFloat because spilling 1 + 2^-24 changes the
source ground-distance result. The ordered comparisons retain their source FP
context. Precision/invalid flag differences remain reported partials, never a
strict pass. No existing judge, probe, expected value or tolerance is changed.
The only existing test registration change is the appended native unit row.

This packet makes no rendering, GPU, N3b completion, story, full-controller,
full-gate or native-app claim. Root must obtain a fresh independent STYLE review
before product consumption, bind the actual lower calls and continue the route.
