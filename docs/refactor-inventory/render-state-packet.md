# Shared render-state migration context

Preparation at Main `ee343ef`, with behavior unchanged from `f6c4e53`.
The fresh full baseline is green. This packet has not changed behavior, claimed a
completed migration, or run a new gate. The prerequisite enforcement candidate is
separately committed and under review; integrate behavior after its qualification.

The outcome is one semantic implementation of packed-blend selection, device
blend writes and depth-state writes, used by native composition and reference
adapters. Keep numeric compiler tables as literals/typed data and device handles
as native identities. Preserve source diagnostics and fixture absence in adapters.
Do not extend queue traversal, matrix algorithms, texture lifetime or Session here.

## Source and existing ownership

`ninja_render.hpp/cpp` exposes packed state, mapped tables, device identity and
ordered command receipts. It mixes fixture availability and instruction addresses
with decisions. `ninja_queue_native.cpp` independently implements packed-byte and
special/table device blend logic. `runtime/native_display.cpp` also maps depth
comparison and writes depth enable directly. Carry those real native consumers
through the shared implementation; native closure must retain every required method.

The native world owns its byte in `queue::State::values.packed_blend`, inside
`Display::queue_state_`. Borrow that byte, preserving its zero initial value.
The legacy reference world uses `NinjaRenderState::packed_blend_at_3ab9890` and
borrows it through `NinjaSpriteQueueRuntime::render_state`. They are separate
runtime worlds, not permission to create competing caches within either world.
Move a canonical owner with every consumer; keep unknown availability separate
from the recovered value. A temporary compatibility interface must forward to the
shared algorithm, remain explicitly open inventory work, and stay natively linkable.
Such an intermediate interface does not certify the entire module as conforming.

Cached original evidence is `reference/inventory-raw/bridge-blend-4030d0.*`,
`bridge-blend-77ec60.*`, `runtime-tables-blend.*` and `runtime-tables-depth.*`.
The symbol lookup receipt identifies `njColorBlendingMode`, its lower variant,
`njSetZCompare` and `njSetZUpdateMode`. Confirm exact assembly ordering where a
new seam changes reads or callbacks. Existing source evidence suffices unless a
specific missing question requires the owned REA/Ghidra runner.

## Invariants at the interface

* Store the selected packed nibble before lower device work, even when it fails.
  Only selectors 0/1 access packed state. Pass the original full-width mode to
  the lower call; masking the stored nibble does not mask the lower table index.
* Modes 0/1 ignore the selector and issue two writes. Acquire the current device
  separately for each write; the first callback may replace or invalidate it.
* Other modes read mapped value first, device/vtable next, and selector table
  last. Modes 12–15 use the actual contiguous depth words; a 56-byte fixture has
  no such backing. Unsupported/absent facts retain their existing source stops.
* Negative HRESULTs are ignored. Missing native output and incomplete callback
  receipts remain explicit failures; command/pending-receipt ordering stays exact.
* Depth comparison masks its index by 7 and preserves device-before-table reads.
  Depth write preserves the full value: the reference unit deliberately sends 42.
  Native bool callers do not redefine that source contract.
* State/stage issue helpers reacquire live device identity and preserve command
  receipts. Their actual consumer closure includes view/frame/texture paths; a
  blend-only oracle is not full depth, texture-stage, GPU or lifetime proof.

## Verification and remaining prerequisites

`reference/production-integration-20261101/render-state-baseline/` freezes fresh
consumer reports and textual consumer candidates, plus exact probe commands/hashes.
Five of its eight oracles pass; sprite queue, queue consumer and game view retain
their declared FP partials. Separate render and native/legacy lifecycle receipts
remain in the parent evidence directory; the latter two retain 402 flag partials
each. Complete lifecycle case outputs are already frozen there. Resolve any other
inherited per-case residual needed by the actual edited consumer closure before
rewriting it; aggregate partial counts alone do not prove preservation.

Keep the existing unit, probe, oracle, fixture and mutant semantics. Locate historical
blend mutation evidence before claiming it was rerun; none is certified by this
context preparation. Discriminating cases include removed device reacquisition,
reordered packed store, changed special modes and broken contiguous mode backing.
Missing depth/stage independent coverage must be established separately as judge
work before those algorithms change. Run affected consumer judges, native closure,
existing journeys and one coherent integration gate after fresh STYLE §12 review.

Build discovery and native closure currently assume flat source paths. Use the
current system module location until any nested-directory build/check migration
is separately evidenced. Do not move algorithms into guest or exclude them to
evade readability or force-load checks. This packet is one system in the complete
campaign; Sonic, reference composition and all other maintained modules remain in scope.
