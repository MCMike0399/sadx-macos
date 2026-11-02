# Native model-matrix core

Base Main is `03efe626`. Source is `b2271ed7` on the supplied worktree,
`packet/model-matrices-native`. Root owns admission, shared-owner hunks and the
combined gate. Root released the serialized slot after its separate Main gate;
the original runtime and the exact patched native caller have now been measured.

The recovered core owns one `model_matrices::State`: unsigned count, 128 retained
64-byte matrices and unsigned limit. `ROOT-OWNER.patch` places that state in the
existing Session and borrows it through Gamma's Context. The patch is tested on
copied source fixtures, without editing shared worktree headers. The measured first-caller state supports
`ROOT-ACTIVATION.patch`. Both patches run in a private compiled native app; shared
worktree Session/Display/PlayerSystem files remain unchanged for Root admission.
The first getter returns false and preserves its destination, as the source does.

GetMMMatrix compares unsigned indices and preserves every destination word on
rejection. Valid calls copy all sixteen words. The owner patch also corrects the
five existing Gamma matrix annotations from 48 to 64 bytes; their C++ allocations
already contain sixteen floats. Five exact symbol override widths are corrected
in the same patch. The original njSetMatrix dossier proves the 64-byte REP MOVSD
extent. Regenerated symbols and the unchanged provenance checker accept all
175,836 owned-byte identities, with no counterpart-guard change.

CalcMMMatrix preserves wrapped requested-count-plus-one and the clamp to 128,
clears count, pushes the existing stack, optionally copies a base, wraps the
frame through the maintained MSVC conversion, traverses once from the root,
optionally copies the requested output and pops. The requested output's 32-bit
byte offset also wraps: a large unsigned index can select an in-buffer matrix.
Undeclared out-of-buffer reads remain an explicit missing-storage boundary.

ScanMotionModel preserves the source layout selection, key interval/wrap rules,
coefficient spills, signed sixteen-bit angular deltas, nullable channels and
live-pose fallbacks. MDATA2's missing position channel ignores UnitPosition;
MDATA3 respects it. Parents loop child siblings; the top-level ignores its root
sibling. Capacity-exhausted pushes and popped-slot bytes use the existing Ninja
stack rather than local identities or automatic rollback.

Numeric njTranslate has its own source accumulation order; njTranslateV remains
the maintained live-pose operation. Numeric njRotateXYZ composes the existing
exact axes in Z/Y/X order and skips only full-word zero angles. Gamma's direct Z
rotation also borrows Display's existing primitives and arithmetic; it introduces
no second matrix stack or numeric owner. njMultiMatrix
uses x87-width expressions with the unrolled accumulation orders and snapshots
only its destination, preserving exact self-alias behavior. Existing primitives
and the animation transition builder are unchanged.

NativeMotionSource borrows existing GammaMotionAssets imported resources, the
actual PInitialize/PSetMotion generated action/motion/arena keys and Session's
ObjectState. It owns no mutable pose, geometry, material, input or animation copy.
Reference failures translate to source boundaries in the guest-only diagnostic
adapter. Missing headers/keys and unsupported layouts remain explicit.

## Qualification retained before the hold

- Development: 1,223 cases, zero value mismatches, 957 individual FP partials,
  including 27 source failure prefixes. Strict exit is 2.
- Final independent holdout: 4,935 cases, zero value mismatches, 3,934 individual
  FP partials and 212 failure prefixes. Strict exit is 2. Every output matrix,
  declared stack slot, count/limit and destination word is observed.
- Real imported Stand and two generated transitions after distinct live pose
  edits: 774 original/native rows, zero value mismatches and three FP partials.
  All 128 matrices, 128 stack slots, destination and count/limit/depth are compared.
- Twelve core/asset compiled faults are caught, including unsigned getter rejection,
  clamping/wrapping, root siblings, MDATA2 flags, arithmetic order, rotation order,
  imported key offsets, generated stride and live-node lookup. Five additional
  compiled getter/rotation binding faults catch wrong owners, invalid-output
  writes, index truncation, wrong axes and rotating the stack instead of the
  destination. Three activation-guard negatives reject unsupported first-state
  claims before compilation.
- The unchanged coordinate oracle retains 23,724 cases, zero value differences
  and 21,240 FP-status differences. Its report does not support a claim about
  every inherited individual partial identity.
- Unchanged input and weld oracles retain 636/1,200 exact cases, including 469/248
  failure prefixes. Their native caller programs and the motion caller pass.
  The 11,144-byte motion and 12,494-byte topology observations retain their
  accepted hashes. No old judge, input, tolerance or compiled fault was edited.
- The private exact-patch native app and force-load closure build from 193
  maintained sources at concurrency two, with copied public controller products.
  The unchanged native closure checker passes. Normal input now reaches
  GammaUpdateFan at frame 1332, exit 1, completed frames 1330. All five getters
  and the direct Z rotation execute; the inspected title capture retains hash
  `c7d02f642ae8e5493f19051e7cf13a06a2d1d103351e72b0576a108b52aba427`.

Partial identities and full observations remain in ignored worktree build
outputs; VERIFICATION.json pins their hashes. The source key fixture declares
four keys per channel and stops at its first undeclared frame read. These are
missing-fact prefixes, not hardware fault-PC comparisons. Synthetic MDATA3 core
sampling is covered; an actual generated MDATA3 caller and arbitrary resource
aliasing remain unqualified. Self-aliased njMultiMatrix is covered.

## Original first-caller state and remaining boundary

The first attempted reference witness could not open the then-absent pinned game
path (exit 134). After Root supplied and released its live pinned directories,
two direct getter witnesses returned the same exact snapshot hash. The final
extended witness executes through all five getters and Z rotation, stopping at
the actual first GammaUpdateFan entry at reference frame 1334. It installs no
native game overrides. Inputs retain their hashes before and after the run;
all writes use a fresh overlay inside this worktree.

There are zero preceding CalcMMMatrix calls. Count, limit, all 2,048 buffer words,
all five Gamma destinations and all four weld scratch matrices are zero at the
first getter. GetMMMatrix(54) returns false with all sixteen destination words
unchanged. Actual getter order is [54, 30, 39, 2, 37]. At the fan boundary, the
only changed destination words are node39 components 4–7: negative zeros from
the recovered Z rotation. Count/limit/buffer, scratch and stack depth remain
unchanged. This is observed caller state, not a BSS-only inference.

INITIALIZATION.json retains exact commands, executable/library/config/save
input hashes and unchanged-input checks. INITIALIZATION-SNAPSHOTS.json preserves
the complete small runtime snapshots; these contain system state, not game
assets. REAL-CALLER.json pins the private native build, command, state and image.
The reference/native frame difference remains 1334/1332; no timing alignment or
actor-frame equivalence is claimed. The packet's ordinary unpatched build retains
the old getter stop until Root applies the shared source patches.

Active PJoin mode 1 still stops at CalcMMMatrix. Its argument selection,
deformation and attachment writes are not implemented or qualified here.
The four persistent weld scratch matrices must retain source bytes when that
branch is recovered; this packet does not create local replacements. The six
attachment positions/directions remain in the existing player allocation.
There is no actor frame, N3b, hardware FP, source archive or full-gate claim.

All 1,418 inherited protections, including 53 locally absent inputs, remain
unchanged. JUDGE-PROPOSAL.json is a finite addition/registration proposal. No
omission refresh or counterpart-guard broadening occurred. New Python helpers,
probe and unit are separate from the source commit. Root must seal the exact
finite delta after independent review. No Main write, push, agent, new checkout,
reserved Task/reset, schedule, storage or provider work occurred.
