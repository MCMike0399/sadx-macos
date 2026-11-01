# Frame and mode foundations at 99bdaf4

Independent read-only review of all nine files, **1,398 lines**, at
`99bdaf4a9e06006aed7f9fcf2c963a6e68cf61d3`. The JSON is the authored inventory fragment;
it pins every reviewed member, consumer, receipt and current dependency. No source
changes or new execution were performed. Main's active gate 97018 remains Root's
responsibility. Product acceptance and milestones are unchanged.

| Exact module identity | Complete members | Judgment |
| --- | --- | --- |
| `src/engine/game_frame_control` | `.hpp`, `.cpp` | Conforming |
| `src/engine/game_frame_control_internal` | `.hpp` | Conforming |
| `src/engine/game_frame_control_native` | `.cpp` | Conforming |
| `src/game/game_mode` | `.hpp`, `.cpp` | Conforming within the shared dispatcher contract |
| `src/game/game_mode_internal` | `.hpp` | Conforming declarations; no native wiring claim |
| `src/game/game_mode_native` | `.cpp` | Open qualification |
| `src/game/game_mode_services` | `.hpp` | Open qualification |

That is **7 conforming files / 5 conforming modules, 2 open files / 2 open modules**.
No demonstrated migration defect warrants rewriting these established patterns.
Open qualification is not an instruction to invent native callees or expand gameplay.

## Semantics and ownership

Frame control preserves the frequency read before the interval multiplier store,
low 64-bit signed-product division by 60, the three mode stores before selected
renderer work, and callback installation after successful setup. Render ratio 2
only changes the interval when the configured factor is 1. Reset flags preserve
short-circuit save/edge/mask reads and mask 68. The limiter queries frequency before
reading callback-mutated timing; each counter callback precedes a fresh previous
counter read. It stores sampled time, rather than accumulating a deadline, and
handles failed frequency with exactly one initialization counter query.

Application owns `frame_mode` before constructing Display and Session. Session
borrows it and owns renderer timing; LogoScene, MovieScene and frame-loop views
borrow those same objects. Native InputState owns reset flags/mask and borrows edge
and save fields. The actual production Input mod reset producer is separate:
callback installation alone does not establish that production invokes this
frame-input callback. In the reference world, NativeGame owns timing/renderer/input
once; invocation-local native views are transport, not authoritative caches.
Split counter knownness and read failures stay in `guest/frame_control_reference`.
Both adapters restore previous thread-local bindings on unwinding.

The shared mode dispatcher preserves signed short callee results, unsigned title
bounds, 32-bit wrapping increments, distinct cleanup/store order, destructor order
4/0/5/7/6/8/9, message clearing after release, same-frame initialization fallthroughs,
callback-mutated mode/load-phase rereads, repeated player queries, sequence
access at use, and the alert/fade/frame-counter tail. Every helper and both public
entrypoints were read. Phase metadata supplies diagnostics, not game decisions.

Session's `flow_` is the native owner. Bootstrap borrows its movie number; title,
save, message and character selection consume the same fields. The reference
adapter borrows NativeGame's mode, GameLoop shared words and GameModeState remaining
facts. Its source sites, pending calls, required returns and missing-fact optionals
stay outside the shared game decisions. `SequenceSelection` is a borrowed actual
sequence; `MenuResource` is opaque here, with native release still uncomposed.
This review does not certify a native allocation identity for that resource.

The private state accessor interfaces and individual link-time STUB functions are
legitimate seams. `NativeServices` has 27 finite named game operations, a virtual
destructor and a real private Session implementation. It owns no source state and
has no generic dispatch key, register transport or fixture availability. Its breadth
alone is not evidence of bad abstraction. Its **current qualification** remains open:
12 cleanup/title operations were added after the tracked mode report. Many named
functions still throw explicitly, including Action/Adventure/Mission and sequence
operations. They must remain unresolved; native story behavior is not inferred.

## Evidence and actionable gaps

All four frame files, the speedrun helper and native host unit match the historical
isolated native source receipt. The portable adapter's complete recorded source
map also matches. The frozen 2,672-row TSV matches its receipt. Original frame
2,672 and limiter 1,799 comparisons report zero mismatches; the limiter excludes
one capped fixture. Historical frame holdout has 2,048 cases, zero mismatches and
identical before/after input hash. Three core faults produced 120/472/182 mismatches;
three native host faults were caught. These are retained results, not reruns.

The current mode header/core, reference adapter, probe and verifier match the
24,000-case, 252-path oracle receipt. The current internal header, native backend
and services **do not** match that receipt. Internal-header changes are declaration
annotations; current backend/services add actual composition. The old semantic
receipt is also stale for the core. Its three mutants caught 1,629/54/10 mismatches;
its 2,048-case before/after holdout shares an input hash and reports zero mismatches.
The current native unit checks only four explicit-stop/prefix scenarios and
installs no services. These limits are why the native backend and services stay open.

Root should bind existing actual product closure/journey evidence to the current
backend, services and Session dependency identities, then supply bounded forwarding
and prefix-failure observations where coverage is missing. Do not replace the
reference oracle with that evidence or claim that link closure proves behavior.
The native `request_file_module` no-op is supported indirectly by 96 original leaf
cases; its direct 4221e0/disabled-printer raw dossier was not located among reviewed
raw files. Caller dossiers establish call sites, not that callee body. This is a
remaining evidence gap, not a demonstrated regression.

Before any future mode rewrite, freeze the full diagnostic result tuple too. The
48-input/256-output mode probe does **not** serialize `nested_source_boundary` or
`pending_call`; the unit checks selected pending-call/availability behavior. Matching
failure verdicts alone cannot establish all original diagnostic identities.

Ignored raw Ghidra dossiers and SDK headers were read from Main and are separately
labeled and hashed in JSON. All three original input corpora exist in Main and
match report hashes, but are absent in this packet; they are inputs, not retained
full output streams. Historical transitive harness fingerprints, complete mode
holdout output streams and current all-forwarder observations are not inferred.
Frame corpus generator metadata also predates its current verifier; that historical
hash is preserved separately from the current dependency audit.

The reviewed cores use integer control/arithmetic; frame FP flags and unused input
EAX are excluded by the original judge. Downstream title/cleanup FP partials and
source-boundary failures remain partial/failing. No physical pacing, stage 63 FPS,
new native/reference journey, whole engine/runtime/Session or campaign closure is
claimed. No tests, builds, oracles, mutations, providers, network/storage operations,
delegates, full gate or push were run. Only this fragment and context are committed.
