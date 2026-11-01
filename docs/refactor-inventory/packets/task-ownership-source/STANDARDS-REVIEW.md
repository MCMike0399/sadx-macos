# Independent Standards / STYLE §12 review

Final reviewed Source: `bceff1bbc10f26636dac592e5a2aac735d746c1a`.
Engine/source packet: `78702dbc03a74f7dde5ac4ed1ca0797de9e628c7`;
immutable before: `bd22ee423ea015f00152a97fa8fc2e1b4034b411`.
Exact Git blobs/diffs were reviewed, not the assigned checkout's older engine.
The original source commit changes 18 paths; the before-to-final range also
contains earlier packet/judge preparation. The final follow-up changes only
`player_auxiliary_effect.hpp/.cpp`; all four engine blobs are unchanged.

**Standards verdict: small changes requested; two P3 findings, no architectural
blocker identified. This is not Spec/fidelity approval.**

1. **P3 — enum naming remains nonconforming.** At Source
   `src/engine/task.hpp:127`, the new `TaskStep` uses `none`,
   `pool_already_initialized`, etc. STYLE §2 requires CamelCase enumerators.
   Complete-module residue also includes `DestructionStage` at line 72,
   `TaskFailure` at line 101, the lifetime/context enums, and
   `src/engine/task_work.hpp:21` (`TaskWorkFailure`). Rename consistently,
   preserving ordinal values and updating actual consumers. The inherited
   cases are whole-module classification debt, not newly invented behavior.

2. **P3 — introduced helper lacks provenance classification.** Source
   `src/engine/task.hpp:403` declares `advance_child_lifetime` without its
   own `NOTSA` annotation; its new definition is `task.cpp:853`.
   STYLE §4 and §12.9 require introduced helpers to be tagged on their
   declarations. Mark this extracted child-lifetime step explicitly; do not
   imply that it is another recovered source entry.

**Bounded module disposition:** both complete modules `src/engine/task`
(`task.hpp/.cpp`) and `src/engine/task_work` (`task_work.hpp/.cpp`) remain
`needs_migration` for the limited standards residue above. Their reviewed
ownership/boundary design is acceptable statically. No broader consumer module
is promoted to conforming.

TaskScheduler remains the canonical graph, pool, free-slot, storage and context
owner. Links remain nonowning; retained handles distinguish recycled generations;
outermost WalkGuard defers reclamation. Graph reconciliation uses a temporary
validation plan. Scheduler transitions and staged, lazy cleanup remain in engine;
the guest header does not implement allocator/scheduler policy. Callback reentry,
saved-next resolution, post-destructor sibling capture, callback clearing before
children, and auxiliary release ordering remain explicit.

PointerEncoder replaces the numeric reference base without replacing allocation
identity. Optional offsets distinguish real offset zero from null links. Weak
targets, interior offsets and saved allocation offsets remain distinct. Byte
knownness preserves unencodable pointer representations and observable stale
headers; it is not a blanket fixture-optional gameplay model. Live payload reads
precede cached reference resolution; metadata writes bypass payload transport.
Native LogoScene supplies its default explicitly; NativeGame installs the narrow
lazy unavailable-default reader. The three symbol overrides explain cooperative
thread naming. Child copying returns semantic steps without adding pose copies.
The final auxiliary result uses plain `TaskStep`, removing added handle retention.
No generic Services framework or new silent callee substitution was found.

Complete files read: the four engine files, `guest/task_reference.hpp`,
`hle/hle_task_bridge.cpp`, `player/player_task.hpp/.cpp`,
`player/player_auxiliary_effect.hpp`, and `game/game_scene_logic.hpp/.cpp`.
Other touched consumers were reviewed at their actual changed bindings and
surrounding call sites. Source PACKET/DEBT, accepted task-judges PACKET and
task-actor-ownership.json supplied bounded context.

No builds, tests, oracles, mutations, runtime, holdouts, providers or network work
were run. Reported 13 units, Bridge/NativeGame/native force-load and four exact
16/40/236/244 ABI streams apply to `78702db`, not final `bceff1b` qualification.
Final API/judge bindings, recompilation, 14+3 fault qualification, fresh reserved
inputs/holdout, separate Spec review and composed gate remain prerequisites.
No source, gate or campaign closure is claimed.
