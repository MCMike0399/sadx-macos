# Native particle task lifecycle

Base Main 76b44198; durable branch packet/particle-lifecycle-native.
Implementation 897f2f02 recovers standard update, custom update, display iteration,
destruction and the reached animation leaf around the accepted canonical owners.
Empty real pools survive scheduler update/display and delayed teardown/reentry.
Nonempty physics, animation and removal are qualified; actual rendering remains
an explicit DisplayParticle boundary. No completed actor frame or N3b claim.

The source retains signed-short counts/indices, binary32 store order, integer
frame/loop words and wrapped descriptor arithmetic, swap removal, captured work
versus reloaded task.awp, buffer aliases and callback reentry. Particle views
borrow the existing arena slots. Work and mutable imported Frame storage retain
their accepted ownership; no copied frame, particle, texture or renderer owner.
The destructor frees three buffers in source order; scheduler-owned work release
and task recycling stay with the scheduler. Root's Gamma global clear is separate.

Qualification, with complete commands/hashes/limits in verification.json:

* Lifecycle: 1,109 development / 2,048 holdout cases, zero value failures and
  237 / 372 FP-status partials, strict exit 2. Both complete work records, exact
  published work identity, all slot/index/frame bytes and lower-call prefixes
  compare. Nine protected-page store faults, null unchecked buffers, signed
  counts, wrap, descriptor sentinels and callback reentry are covered.
* Nine compiled lifecycle faults are caught. A capacity-corruption fault passed
  the initial observer and fails the stronger complete-work observer; the
  independently evidenced observer change is fcbfa8ff. Product source is unchanged.
* Initializer: unchanged 1,145 / 2,048 cases and every individual 559 / 1,115
  FP partial record and original-output stream are preserved. All seven existing
  compiled initializer mutants are rerun and caught; their original receipt is
  restored byte for byte. No old oracle/tolerance/domain/mutant was edited.
* Three native programs pass: actual scheduler/arena lifetime and nonempty state,
  imported Gamma declaration reentry/pending tasks, and real Gamma/PInitialize
  caller reaching PSetMotion. The obsolete callback-stop unit assertions were
  adapted separately in 0657b5a7, retaining all initializer assertions.
* Fresh local source build and updated force-load native closure pass at -j2.
  Only public SDL/libusb dependencies were copied, with symlinks dereferenced.
  Plain-float physics loses 108 values on unchanged development inputs; readable
  ExactFloat arithmetic is retained. Layers, ownership, readability, hygiene and
  proposed-symbol provenance pass; five recovered profile ranges have zero
  UNHANDLED or undetermined overlaps.

SOURCE.md retains source roles/order and reproducible destructor seed evidence.
BINDING.md gives the exact typed arithmetic/rendering/lifetime contracts. Root
owns Display/Session/PlayerSystem/Gamma release integration; none was edited.
REGISTRATION.md and proposed-judge-inputs.json give finite registration changes.
Canonical source/judge/unit/symbol inventories and all inherited protections are
untouched, including the 53 ignored artifacts absent locally. Full gate and
independent review are Root-owned and were not run. No other checkout, subagent,
push, reserved Task/reset work, schedules, storage or provider change was used.

Main's accepted normal-input frontier remains PSetMotion at frame 1332. The
optional local app baseline stalled without output and was terminated; it supplies
no route/image evidence. Rendering, spawning, texture-load flags, Root's Gamma
release binding and arbitrary malformed/cross-allocation aliases remain open.
