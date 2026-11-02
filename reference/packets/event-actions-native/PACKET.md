# Native event-action admission candidate

Base `0e56c5d037b66f730b263400fc4a29afecc76483`; branch
`packet/event-actions-native`. Implementation candidate for Root review, not N3b
acceptance. No app run, full gate, push, schedule, storage or reset qualification.
Gamma setup remains the retained app frontier. No sibling/Root runtime files changed.

## Shipping scope

`player/event_actions` implements SetInputP, EV_SetAction and its actual append
callee, event-work initialization, EV_ClrAction and its actual cleanup callee.
Only the reached fixed CAlloc calls (1 × 124 and 1 × 24) are composed with the
existing TaskWorkArena; this is not a general CAlloc recovery. Native composition
and the reference-only ABI probe execute the same game core. The reference probe
wraps real native arena allocation/release; no successful queue is simulated.

One canonical `PlayerEventActionNode` stores signed mode/link bytes, copied
CharacterPack object/motion identities, the borrowed regular texture list and exact
speed bits. It retains no mutable NJS_ACTION container. Object is loaded/stored
before motion is loaded, including overlap with the new node. Resource IDs remain
scoped to the canonical CharacterPack borrowed by EventActionsContext; future
animation consumers must use that same pack and existing mutable scene owners.
No animation, pose, registry or motion owner is introduced here.

The actual primary allocation's event reference owns the event allocation. Its
mode/timer bytes and head/old/path links remain canonical arena state, including
compatibility with the raw event allocation already made by PInitialize. Typed
node payloads use the one authorized TaskWork member. Work/node payloads do not
own their own allocation. The State retains stable node/allocation identities until session
end; freed allocation identities are invalid even if an arena offset is reused.
This intentionally retains typed tombstones for stale raw preview-tail borrows;
it is not another event list. Preview globals are never cleared by queue cleanup.

Append captures the destination head/next slot before allocation, publishes even
a null allocation, then faults on the first node store if allocation failed. Clear
captures next before free, reads old after list frees, clears head before old free,
and finally clears timer. Null task wrapper guards are preserved; reached null
primary/event dereferences are explicit failures. No missing event is initialized
implicitly by EV_SetAction.

## Source and verification

Fresh owned REA/Ghidra dossiers, exact ranges/hashes and field layout are in
`source-context.json`; raw dossiers/snapshot are ignored under
`build/event-actions/rea`. The one runner used a 2 GiB maximum and closed normally.
No upstream source was copied. Exact observations/results/commands, setup failures
and final source hashes are in `qualification.json`.

Final original comparison: 2,630 development cases and 4,630 final holdout cases
pass. The holdout has 4,000 fresh seeded cases plus the same 630 discriminating
fixed cases, after the final owner correction. Seven compiled mutants are caught.
Observations include return/stop, allocation/free call order and arguments, null
allocation publication, every modeled node field and padding, full primary/event snapshots, links, old/timer/mode state,
registered input fields and reentry mutations. These are void functions; incidental
caller-saved register values are not API return values. MAlloc/FreeMemory internals
are existing allocator boundaries. Duplicate free stops at that boundary in both
adapters; this does not qualify original heap-corruption behavior. Beyond-registry
SetInputP memory is explicitly unavailable; byte truncation into slots 0–7 is tested.

The source-only composed test uses actual TaskScheduler, TaskWorkArena,
PlayerTaskState, Registry.primary, CharacterPack and accepted preview::update_actions
through PreviewActionCalls. It covers actual nodes/tails, distinct registered and
captured Miles primaries, primary replacement, the captured primary's live event
replacement, source copy overlap, failure publication/counts, cleanup, stale tails,
scheduler destruction and reentry. An event allocated by the existing startup
path is admitted/cleared without a duplicate owner. No PE, DLL, guest, HLE or Unicorn
is linked or opened by this native program. Task-native and the old preview lower-
stop unit also pass as separate binaries. Existing fixture-only node definitions
must stay in their separate tests/probes, never enter a production link.

The existing preview core is unchanged; its header changes only comments and moves
the two recovered entry annotations to their canonical declarations. Its 4,013
original cases rerun with the identical input/observation digests. Existing 7,013
holdout and 13-mutant evidence is preserved and was not rerun. Readability has zero
regressions; provenance, layers and unique owners pass. Profile checks cover seven
actual source ranges with zero UNHANDLED/undetermined overlaps.

## Root integration

1. Add `src/player/event_actions.cpp` to `native-sources.txt`. The existing
   `src/runtime/native_*.cpp` wildcard includes `native_event_actions_context.cpp`;
   core source/probe/oracle discovery already finds the new files. Append
   `event-actions<TAB>` to `tests/unit/manifest.tsv`. Default rules give
   `build/event-actions-test`, `build/native-event-actions-probe.dylib` and
   `oracle-event-actions`, with default EXE/probe inputs. No oracle-manifest row or
   symbol override is necessary. Root owns additions-only judge registration and
   inventory reconciliation; existing protected inputs were not edited.
2. Integrate the six-line `task_work.hpp` forward declaration/payload delta and
   rebuild all consumers. This changes the host TaskWork type, not arena layout or
   scheduler/arena algorithms. Keep node fixture programs separately linked.
3. Own one event_actions::State alongside the existing preview ActionState, with
   a lifetime covering every retained tail borrow. Construct EventActionsContext
   from the scheduler's actual arena, PlayerSystem.registry, canonical combined
   selection CharacterPack and these borrowed EventPrimaryFields operations:
   `store_secondary_mode(primary, byte)` assigns `primary.secondary_mode_at_1`
   directly (no prior optional read); `counter(primary)` validates and returns
   `*primary.source_counter_at_c`. Missing counter facts throw the named boundary.
   These adapter accesses intentionally stay out of the new semantic core.
4. Supply the six actual regular texture list pointers to PreviewActionCalls,
   indexed by preview slot. Canonical `(character,binding)` pairs are
   `(0,0), (2,0), (3,1), (5,0), (7,1), (6,0)`. Bind the existing imported sequences;
   action index equal to length is the null terminator. A local PreviewActionCalls
   instance per update captures primary state AND allocation before lower calls;
   action_head reads that captured allocation's LIVE event reference. Do not
   reconstruct it from Registry or recapture task->primary_work after a callback.
5. Call preview::update_actions from its accepted warmup-gated draw position.
   The adapter reads the Miles counter from live Registry.primary[1], which may
   differ from the captured actor primary. Do not clear ActionState tails on
   cancel, teardown, initialization, cleanup or a failed append.
6. `player_startup.cpp::bind_work` already inlines event initialization after the
   motion binding and before contact allocation, advancing Stage::ObjectBound.
   The new queue works with that same allocation now. Root should replace that
   existing source block with event_actions::initialize at the same position when
   migrating the shared startup seam, preserving its existing failure diagnostics
   and unchanged startup oracle. This worker leaves that sibling prerequisite
   intact. It does not add a second event allocation or call initialize from append.
7. Sonic can separately query event_work, move_mode and has_path. The actual
   action head is +0x40; path head is +0x4c. Never alias the legacy optional
   action_list_at_4c observation to the real action queue. Fixture unknowns stay
   at adapters. Actor event-controller behavior, PSetMotion/animation stepping,
   stage/scripts and rendering remain explicit separate work.

Reproduce the focused native build and original comparisons with the exact commands
in qualification.json; all outputs stay under this worktree's ignored build tree.
Root arranges independent review, app bindings, connected N3b journeys and the gate.

## Root review corrections

Review 028f986d accepted the bounded core with two P3 findings. Root reformatted
the probe/unit and Python witnesses, proving unchanged C++ token sequences and
Python ASTs before adding sentinel cases. All 2,630 development and 4,630 held
inputs remain exact prefixes, followed by 630 distinct nonzero-primary cases.
The expanded 3,260/5,260 comparisons pass, as do the native composed program and
eight mutants. A new unrelated-zero-store fault passes every old development case
and fails on the new sentinels. Product source is byte-identical to b05ab74.
The supplement is review-corrections.json; native app/integration remain pending.
