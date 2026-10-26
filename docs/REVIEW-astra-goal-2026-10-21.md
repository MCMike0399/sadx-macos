# Astra goal execution review

Reviewed October 21, 2026, against the complete native Sonic story request issued
October 20 at 07:49 Mexico City time. Committed comparison:
`0f8154a..e6917c8236461eb2788822afa40462d5e242d588`. Later uncommitted work is
identified separately. The implementation session continued during this review.

## Assessment

Keep the implementation and its verification infrastructure. The goal execution
has produced a real assets-only native application with startup, the intro movie,
title, saved-file navigation, confirmation, MainMenu and delivered menu audio.
An independently rerun, isolated build also creates and reloads saves, including
from an initially empty native profile. These are connected product capabilities,
not merely recovered functions.

The requested playable route is still ahead. Native CharacterSelect actors,
Sonic gameplay, stages, bosses, story transitions and measured gameplay at 63 FPS
are not established. Earlier recovered physics cannot simply be added to the
production archive: parts still depend on reference-only interfaces and code.

The next improvement is to integrate complete, source-backed caller chains using
the reviewed work already available, while strengthening the checks of the
composed application. More workers or larger local oracle case counts alone will
not resolve incorrect ownership bindings or unimplemented runtime callees.

## What the user wants

There are two related scopes:

1. The long-term product is a faithful, readable, maintainable native Apple Silicon
   SADX port suitable for speedrun practice and future opt-in extensions. It builds
   from maintained source and runs with assets imported from the user's own media.
   Original EXE/DLL execution and an x86 fallback belong only to the development
   reference. [GOAL §§1–3](GOAL.md),
   [ADR-0005](decisions/ADR-0005-native-source-and-executable-independence.md),
   [ADR-0007](decisions/ADR-0007-readable-game-code-and-extensibility.md).
2. This goal's acceptance route is normal-input startup/save → Chaos 0 defeat →
   Emerald Coast completion → train to Mystic Ruins → Egg Hornet defeat and
   progression → Wind Stone placement → Windy Valley completion. Supporting
   requirements are correct gameplay systems, music/SFX/relevant voice, physical
   Switch 2 Pro and keyboard input, measured 63 FPS gameplay, and the pinned
   Input/Speedrun behavior including CCEF, EHCF and Skipma. The original request
   explicitly makes repository steps an inner loop, not the overall stopping rule.
   [Primary request and session references](RESEARCH-astra-goal-agent-history-2026-10-21.md).

This route is neither a request for an easier substitute game nor a requirement
to finish unrelated entire-game features before the next reachable checkpoint.
Recover every behavior actually required by the connected route, including the
real menus and actor callbacks that precede it.

## Progress established across the goal

| Area | At the start | Current established result | Remaining boundary |
|---|---|---|---|
| Native execution | No integrated native startup; an earlier logo fixture did not prove boot | Real startup/save/task/resource setup, native logos, movie, title and menus; source-only build records | Full gameplay behavior and runtime independence across the requested route |
| Native movie | Not integrated | Decoded picture/audio, normal input skip, complete intro evidence | The documented two-present skip offset and broader event/video timing remain separate fidelity limits |
| Native menus | Not integrated | FileSelect, real saved-data navigation, confirm/cancel, MainMenu, title return and reentry | CharacterSelect initialization/actors and normal Adventure entry |
| Persistence | No connected native save path | Fresh-save creation/restart in isolated WIP build, also independently tested with zero existing saves | Finish integration gate/commit and preserve the empty-profile case in the suite |
| Audio | Platform support absent or disconnected from game callers | Imported music/voice/SFX delivered through actual native menu callers | Gameplay positional audio, changed pitch, listener and queued-voice paths |
| Physical input | Unqualified Switch 2 Pro | USB buttons, sticks, triggers, reconnect and user-confirmed rumble evidence | Qualification through the completed gameplay path; no new claim for Bluetooth or multiple identical controllers |
| Reference route | Controllable Chaos 0 without a proved defeat | Chaos 0 defeat and clean replay to Emerald Coast; live continuation now completes Emerald Coast and returns to Station Square | Replay the new completed segment cleanly, then train/Egg Hornet/Wind Stone/Windy Valley |
| Native route | None | Menus and persistence only | All requested native boss/stage/story gameplay |
| Performance | Reference stage throughput below target | Native limiter exists; no measured native gameplay result | Gameplay frame-time distributions at the pinned cadence |

Evidence: [startup integration](../reference/production-integration-20261020/native-startup-integration.json),
[title closure](../reference/production-integration-20261020/native-title-production-closure.json),
[MainMenu integration](../reference/production-integration-20261021/native-main-menu-connected.json),
[fresh-save closure](../reference/production-integration-20261021/native-fresh-save-production-closure.json),
and the [history investigation](RESEARCH-astra-goal-agent-history-2026-10-21.md).
The fresh-save closure was uncommitted at the review boundary.

The live reference evidence is in the ignored local
`reference/story-route-20261020/durable-continuation/run/report.json`.
Its explicit check at frame 27694 records normal return to Station Square with
control restored; victory and score images were inspected. This moving exploration
report is not a completed deterministic replay or native-gameplay proof.

## Standards and architecture review

### Improvements to retain

The production archive has an explicit native source selection and does not link
guest/HLE or Unicorn. The shared renderer accepts native resource descriptions;
texture snapshots retain image ownership independently of later pool release.
The native session owns the principal save, bootstrap, audio and texture state.
Task callback lifetime fixes retain task ownership across callbacks that clear
their canonical handle. Source quirks and unimplemented operations are generally
documented and exposed rather than silently replaced.

These are meaningful architecture improvements. The many borrowed state records
and service interfaces often preserve source aliasing and observable call order;
their existence alone is not evidence of over-abstraction.

### Native player reuse requires a real interface migration

The existing [Sonic player](../src/player/sonic_player.cpp) still performs
address-based dispatch and calls `get_player_acceleration_ground`, whose
implementation is in
[the guest adapter](../src/guest/player_acceleration_ground.cpp).
Its reference-shaped runtime and optional fixture facts do not satisfy STYLE's
shipping interface rules. The present native source list includes input/heading/
coordinate support, not a complete Sonic gameplay implementation.

This is inherited debt exposed by the next native caller, not a regression in the
new menu work. Recover or migrate the actual required player dependencies under
their existing judges and bind them through semantic native owners. Adding the
guest archive to make a link succeed would violate the product definition.

### Actor, camera and resource composition are the immediate technical barrier

The private preview, camera, lighting and Sonic packets contain useful reviewed
work, but their limits matter. The Sonic packet covers a post-initialization
constructor tail and destructor, not the entire actor entry. It still needs real
collision, texture, motion, attachment and rendering dependencies. Full animation
table/payload coverage is not established.

[CharacterPack::object](../src/engine/character_pack.hpp) returns an asset
description by value. Sonic's shared sibling relationships, welds and deformation
need stable mutable runtime identity derived from those assets. Bind that identity
to canonical owners rather than treating temporary resource descriptions as live
actors. Source CharacterSelect creates other preview actors as well as Sonic;
recover their actually reached lifecycle/render behavior, without replacing
required actors with static poses or expanding prematurely into all their gameplay.

The [application input bridge](../src/apps/sadx_native.mm) currently supplies an
absent gameplay camera, explicitly justified for startup. Gameplay needs the real
camera owner and camera-relative input checks. Renderer view state is not a
substitute for that gameplay owner.

The private lighting packet also distinguishes preview mode 2 from ordinary stage
mode 0. Its mode-0 D3DX direction-normalization dependency remains explicit;
preview lighting proof does not qualify that stage path. Bring the actual source
normalization behavior into the stage dependency list rather than reusing raw
directions or an unqualified reciprocal-square-root approximation.

### Readability remains a route dependency

New code is substantially more semantic, but existing game-tier debt barely moved:
recorded address identifiers declined from 1,505 to 1,499, and hex literals from
10,836 to 10,828 between the starting and pinned STATUS snapshots. This is not a
reason to start an unrelated mass refactor. It explains why recovered physics
still requires migration when the native actor reaches it.

One concrete new STYLE §§4/8 issue remains in
[MainMenu](../src/game/main_menu.cpp): unnamed numeric choice dispatch and
character-selection modes. Name these source-valued concepts as the caller is
extended; preserve invalid-value handling and unchanged verification.

## Specification and verification findings

### P2 MainMenu visual verification accepts black frames

The required [MainMenu checker](../tools/journey/check_native_main_menu.py)
validates PPM headers and sizes, but not the menu's drawn contents. A controlled
experiment supplied its genuine recorded state JSON and unchanged save bytes,
then replaced every one of its ten screenshots with a solid black image.
The complete checker passed. The parent reviewer repeated the experiment.

Local reproduction:
`python3 /private/tmp/sadx-review-main-menu-black-repro.py`.
Parent result: `/private/tmp/sadx-review-main-menu-black-pjuly5vr/result.json`.
No game launched and no production source was changed in this experiment.

This is a real regression-test defect under GOAL §3.1, not evidence that the
application currently draws black. Current screenshots were independently
inspected and show the menu correctly. Promote already established stable
reference frames or regions into the protected checker, keeping semantic state
assertions. A draw-suppression mutation must fail. Retain raw frame/timing facts
when comparing equivalent checkpoints; alignment must not conceal a timing error.

### P2 Generated progress omits native journey results

[record_gate.py](../tools/gate/record_gate.py) parses only R1–R6 journey lines.
[gen_status.py](../tools/gate/gen_status.py) unconditionally says native N1–N5
remain unverified, even though the gate now requires native checkpoints.
The same status page's narrative says those native checks passed.

Correct the recorder/generator from actual machine-readable native results and
identify N3 as partial until CharacterSelect works. Update stale current-architecture
descriptions that still call the native session future work. Do not hand-edit
generated status or change milestone acceptance definitions. This is a bounded
reporting correction that prevents future agents from restarting old work.

### Fresh installation works in the reviewed build but needs lasting coverage

The required [fresh-save journey](../tools/journey/check_native_fresh_save.py)
requires imported saves 01 and 02 and creates 03. An independent additional
experiment began with an existing empty native profile, created 01 using normal
keys, restarted the process, and loaded 01. It passed; its bytes matched the fresh
03 save from the populated-profile journey.

Thus there is no reproduced empty-profile runtime failure. Preserve this successful
case as a protected regression when finishing the save checkpoint. It should not
remain dependent on this review's temporary harness.

### Remaining fidelity claims must follow reached gameplay

The source gate's zero UNHANDLED patch overlaps applies to declared recovered
ranges; it is not proof that the complete pinned profile is implemented natively.
The CCEF and in-game Skipma helpers in [speedrun.cpp](../src/engine/speedrun.cpp)
have no production callers in the inspected source. Native movie skipping and
limiter work do not establish those gameplay hooks or EHCF progression.

Likewise, menu audio delivery is real, but
[mac_sound_effects.mm](../src/platform/mac/mac_sound_effects.mm) explicitly rejects
variable-frequency playback, positional sound, listener updates and queued voice.
Implement and verify the branches reached by native gameplay as part of that
connected checkpoint.

The pinned STATUS contains 99 passing, 70 partial, ten failing and nine missing
oracle reports. These counts do not describe the fraction of a playable game.
Triage failures against the actual upcoming call graph and observed runtime inputs.
Resolve or independently bound relevant differences before claiming fidelity;
preserve unrelated failures honestly without making their wholesale cleanup a
precondition for all further integration. An oracle exit zero can still represent
a partial verdict, as the prior file-select display rerun demonstrated.

## Agent workflow and why progress is still slow

The [primary-source history investigation](RESEARCH-astra-goal-agent-history-2026-10-21.md)
records 45 commits in this goal interval: 38 judge commits, six native features and
one reference-memory fix. The diff contains 20,492 added source lines out of 75,024
total additions. Separate judge commits are mandated; their count alone is not
evidence of wasted work. This is a different pattern from the historical 1.7%
game-source constant-migration series described in ADR-0008.

The inspected main session issued 58 direct spawn calls, 43 with review task names,
while reusing long-lived implementation workers. There is no evidence that simply
increasing delegation solves the current constraint. Review found real defects,
including task lifetime, texture counts and active/fallback filename ownership.

The expensive recurring problems are:

1. **Correlated implementation and verifier mistakes.** The quick-open dialog
   implementation and local oracle both used the wrong fade/counter field.
   Full native/reference images caught it; `b196129` separately corrected the
   judge. Earlier counter and persistent-story flag mistakes had the same shape.
   Verify canonical-owner maps and composed behavior independently of the worker's
   local semantic interpretation.
2. **Private work waiting for integration.** Useful actor/camera/message/save
   packets accumulate while the main agent resolves live ownership and lower
   dependencies. Maintain one short next-checkpoint dependency list with owner,
   actual caller, packet identity and next integration check. Assign workers to
   close independent entries, and consume completed work before duplicating it.
3. **Build and prerequisite friction.** Stale packet headers/objects, duplicate
   target names, missing build dependencies and a windowed smoke stall consumed
   iterations. Preflight affected builds and registrations; keep one owner for
   shared artifacts. This supports proportionate verification, not weaker tests.
4. **Policy drift.** A recent worker instruction declared physics parameters must
   be compiled literals, citing STYLE §5. That section concerns seams; STYLE §8
   and ADR-0007 §5 explicitly permit physics-parameter content tables in imported
   packs. A compiled semantic table is an implementation option. Typed imported
   content is also permitted. Classify data from evidence instead of imposing a
   nonexistent categorical prohibition or importing opaque code-state blobs.

Model attribution is limited: Sol workers contributed throughout the Astra-led
phase, and some source was integrated after its authoring phase. This review
supports workflow changes, not a defensible Astra-versus-Sol quality ranking.

## Critical path and steering

Finish the fresh-save integration boundary, then consume the preview/camera/
lighting/actor packets through the real CharacterSelect caller. Build a bounded
dependency map of that checkpoint, including callbacks, shared owners, resources
and active profile replacements, before opening more independent recovery tasks.
Use source guards and observed reachability to scope the work; keep every required
operation faithful and report unimplemented paths explicitly.

After real character selection, follow normal fresh-story entry into native
Chaos 0: event/cutscene transitions, stage assets, geometry, camera, collision,
player update/render, boss behavior, audio and save progression. A useful first
gameplay checkpoint is controllable Sonic with source-matching movement/jump/
attack/hurt behavior and the boss active, followed by defeat and natural return.
The full goal then continues through Emerald Coast and the remaining route.

In parallel, preserve the reference observer and its single input owner. Freeze
the achieved Emerald Coast continuation into a rerunnable normal-input script
with state assertions and source/profile identities, verify it from a clean start,
and extend the later story. Its role is to provide reusable judgments for native
integration, not to stand in for product progress.

Keep production source-only builds, meaningful mutants, independent reviews,
canonical ownership, protected journeys and actual audio checks. Measure gameplay
performance under a separately recorded workload once gameplay exists. Preserve
physical-controller evidence and ask for new human interaction only where new
hardware-dependent behavior needs it.

The [continuation prompt](PROMPT-native-story-continuation-2026-10-21.md) turns
these findings into an execution request without changing the acceptance target.

## Verification performed for this review

- Independent standards and specification reviews, plus primary session-history
  research. System-level coverage across the goal with targeted detailed source
  inspection; this is not a claim of exhaustive proof of every added line.
- Verified the isolated fresh-save executable against its recorded SHA-256:
  `9fb670057740bdd8cadd0944b7f505174be0f79e3ce8767a7b8b7327861875c7`.
  Copied it into a private review directory to avoid a moving build artifact.
- Reran the committed fresh-save checker: 1,300 create frames plus 2,150 restart
  frames; exit zero, persistence/reentry/resource/audio checks and saving-region
  comparison passed. Inspected actual saving and MainMenu images.
- Additional normal-input empty-profile experiment: 1,300 create frames plus
  1,000 restart/load frames; both exits zero. Both fresh-save paths produced
  SHA-256 `5eb5df01ffe807fb2c05681941f53dc98924ed3b5f72900a4e7dac88e42fdffe`.
- Reproduced MainMenu black-frame acceptance using genuine recorded states.
- Reviewed source-only build records and physical-controller evidence. No fresh
  full build, full gate, gameplay timing measurement or hardware qualification
  was performed in this review. The active implementation work was preserved.

Fresh run evidence and one-off harnesses remain local under
`/private/tmp/sadx-whole-goal-review-p_v9xdhe/`; no game images/assets or binaries
are added to the repository. These temporary results support the named review
experiments, not a new recorded integration gate.
