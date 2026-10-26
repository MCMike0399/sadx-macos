# Astra goal: execution history and integration lessons

Investigated 2026-10-21. This note reviews the complete connected-story goal that
started on October 20, including its earlier project context and its continuation
after the model switch. Committed-code comparisons are pinned to
`0f8154a..e6917c8236461eb2788822afa40462d5e242d588`. The implementation session was
still running; observations of later private packets and uncommitted work are
explicitly separate. No application tests were rerun for this history investigation.

## Sources and attribution limits

The primary history is the local Codex rollout
`~/.codex/sessions/2026/10/20/rollout-2026-10-20T07-49-31-01a15993-0a6d-7993-9a53-bf3c56eb243b.jsonl`
(abbreviated **S** below). Timestamps below use Mexico City time, UTC−06:00. The
rollout itself stores UTC. Line numbers identify records in the inspected append-only
file. Worker `story_route` has a separate rollout ending
`01a15993-900b-7ab2-922f-7155040fa5a8.jsonl` (**W**).

Other primary evidence is committed source/history, [HANDOFF](HANDOFF.md),
[GOAL](GOAL.md), [STYLE](STYLE.md), the frozen packet manifests named below, and
existing reports. Session commentary establishes what an agent reported at a
particular time; a report or source inspection is stronger than a commentary claim.
Encrypted inter-agent message bodies were not interpreted. Token records were not
summed: cached input, repeated cumulative records, child usage and context replay
make a raw sum unsuitable as a measure of cost or work. No model-specific quality
ranking follows from commit authorship or timestamps.

## The intended outcome

The request was more specific than the repository's general milestone M1. It asked
for one normal-input native Sonic story route: create/load save, defeat Chaos 0,
complete Emerald Coast, take the train to Mystic Ruins, defeat Egg Hornet with the
correct progression, carry/place the Wind Stone, and complete Windy Valley. It also
required actual gameplay at the pinned 63 FPS target, keyboard and physical Switch
2 Pro support, music/SFX/relevant voice, and the Input/speedrun profile including
CCEF/EHCF/Skipma. Compiled native execution and imported assets must replace original
EXE/DLL execution; readable source and faithful behavior are acceptance conditions.
[S:11, original user request; S:16, persistent goal creation.]

The product therefore is both a useful speedrun-practice game and a maintainable
community codebase. A collection of exact functions, a functioning reference game,
or a native menu alone cannot satisfy that purpose. Conversely, requiring complete
recovery of every unrelated game subsystem before the next connected route is also
inconsistent with the request. [GOAL §§1, 3](GOAL.md),
[ADR-0005](decisions/ADR-0005-native-source-and-executable-independence.md),
[ADR-0007](decisions/ADR-0007-readable-game-code-and-extensibility.md).

## Establishing the scope and timeline

S begins October 20 at 07:49:33 with `gpt-6-astra`, high reasoning. The initial
checkout was `0f8154a`, gate 177. The complete goal was created six seconds later,
without a token budget. The root switched to `gpt-6.1-sol`, medium at October 20
22:57:34, on the user's request to push current work; it used high reasoning from
23:03:07. Workers were already GPT-6.1 Sol medium before that switch. Some worker
work started before a later integration commit; the root model at commit time does
not establish who recovered or wrote each module. [S:8,16,85,13229,13386.]

The earlier October 10–19 work explains why this goal was needed. The October 13
hybrid runtime supplied a working reference, but subsequent leaf ports, constant
migration and gate runs stopped changing the visible game. The October 16 review
identified missing 3D rendering, a wrong Direct3D FPU premise and repetitive
verification. Its dated counts are historical findings, not newly reproduced
measurements in this note. The October 19 research explicitly diagnosed composition
as the immediate bottleneck. [October 16 review](REVIEW-2026-10-16.md),
[October 19 research](RESEARCH-breakthrough-2026-10-19.md),
[ADR-0008](decisions/ADR-0008-visible-journeys-and-proportionate-verification.md).

| Local date/time | What the goal execution established | Evidence and limits |
|---|---|---|
| Oct 20 07:49–10:38 | Native startup integrated into `Start → LogoInit → Logo → Movie`; 16 state/image checkpoints matched the reference. | `640522f`; HANDOFF gate178. This replaced the earlier logo fixture with actual startup/save/task/resource callers. |
| Oct 20 morning | USB Switch 2 Pro mapping/sticks/reconnect qualified through SDL3/libusb; user confirmed rumble. | S:705–1350; HANDOFF gate178. Later polling revisions have automated coverage; this is not a fresh hardware test of every later build. |
| Oct 20 09:24–11:06 | Reference defeated Chaos 0, reached Emerald Coast; a connected replay reproduced 42 checkpoints. | S:2126,2733,4310. Reference only. |
| Oct 20 11:22–12:44 | Native movie/input/audio/rendering connected; full 3,233-frame intro completed with delivered audio, then explicit cleanup stop. | `ba931d7`, gate180; S:4760,5476,6399. Native gameplay remained absent. |
| Oct 20 afternoon/evening | Cleanup, texture/task ownership, lighting, banks, title and Start SFX connected. Title image matched the reference at the same title timer. | `eba0377`; HANDOFF native title. Two-present movie-skip offset was explicitly retained. |
| Oct 20 16:52 onward | User queried observer RAM; accumulated legacy draw history was diagnosed and bounded, preserving the live reference route. | `17c728a`; HANDOFF observer entry; S:9712,9919. Separately reviewed live cleanup reduced reported footprint from 9.2 GB to 1.7 GB; this was reference infrastructure, not native gameplay progress. |
| Oct 21 08:22 | Native FileSelect navigated real saves with option music, voice and SFX. | `2e8309a`; HANDOFF. Confirmation still stopped at an explicit dialog boundary. |
| Oct 21 10:20–11:32 | Dialog confirm/cancel/reentry and then source MainMenu became connected and required journeys. | `ecbd088`, `fe178cd`; HANDOFF. Adventure stopped at CharacterSelect initialization. |
| Oct 21 12:44, after committed checkpoint | Implementation session reported fresh-save creation, restart/reload, both cancellation paths and reentry passing the connected checker. | S:22004; uncommitted integration/private proof at inspection. Full gate subsequently hit integration/build problems; do not present this as the frozen commit's completed gate. |
| Oct 21 15:53, live continuation | Session reported reference Emerald Coast completion and post-stage event. | S:22829. New live reference claim; train/Egg Hornet/Wind Stone/Windy Valley and native gameplay still unproved. |

This is substantial native integration progress. Repeating the October 19 conclusion
that there is no real native startup would be wrong. Equally, none of these entries
establishes the complete requested native playthrough.

## How the agents actually worked

The root immediately used three independent lanes: native logo/startup, normal-input
story reference and platform audio/controller delivery, while it owned application
integration. Through the inspected continuation there were 58 direct spawn calls,
43 with `review_` task names, and 57 unique task names; all requested Sol 6.1 medium.
These are tool-call counts, not simultaneous agents or a measure of useful work.
Repeated follow-ups reused long-lived workers. [S:39,45,51 and subsequent spawn
records; S:1667 explicitly reports all four slots in use.]

Later recovery became private packet delivery: workers supplied source, private
probes, oracle/holdout/mutation evidence, hashes, boundaries and handoffs while the
root held shared source stable for integration and the gate. This prevented many
shared-build collisions and made ownership explicit. The handoff warns against
copying old whole headers from packets; it requires surgical integration of canonical
owners instead. [HANDOFF entries: native title, FileSelect, dialog, MainMenu.]

The separate story worker was also reassigned to CharacterSelect resource/camera/
actor recovery while the reference observer was held at Emerald Coast checkpoint
23255. A new `reference_ec_continuation` worker was spawned October21 15:29. This
is evidence of dependency-based reassignment, not a continuously staffed gameplay
lane for the entire interval. [HANDOFF latest active-workers entry; S:22384.]

The committed interval contains 45 commits: 38 `fix(judge)`, six `feat(native)` and
one `fix(hle)`; the diff changes 664 files with 75,024 additions and 276 deletions.
These are exact `git log`/`git diff --shortstat` counts for the pinned interval.
Many small judge commits register source files, dependencies and reports while
larger feature commits integrate several systems. The repository explicitly
requires separate judge commits. This ratio alone neither proves wasted work nor
justifies removing verification. It does show why commit count is a poor progress
metric. [Git range above; JUDGE](JUDGE.md).

## What worked and should be preserved

1. **Real application composition found defects that isolated tests missed.** A
   packed-texture precision mismatch was found by native/reference logo pixels;
   startup aliases were exposed with nonzero distinct state; the FileSelect story
   panel exposed a wrong persistent-story/event-work owner. The corrected panel
   eliminated 17,919 differing color-channel bytes. [S:1718,2960,15935,16132;
   HANDOFF native FileSelect.]
2. **Independent review caught consequential errors.** Examples include pinned
   new-save rumble/CRC defaults, cleanup texture reference counts and active versus
   fallback SaveName ownership. These are concrete reasons to keep the required
   fresh reviews, not merely ceremony. [S:581,7230,17572; HANDOFF gate178/dialog.]
3. **Explicit stops prevented fake progress.** Movie cleanup, FileSelect dialog,
   MainMenu and CharacterSelect were named boundaries. Audio readiness was not
   disabled to evade bank loading. Recovered partial actor packets explicitly
   exclude a complete player-factory/gameplay claim. [S:6978,7704,17012,19919;
   Sonic packet cited below.]
4. **Clean source-only builds proved a separate property from behavior.** Native
   startup/movie/title/FileSelect/dialog/MainMenu were repeatedly built without
   original EXE/DLLs or prebuilt game objects and run from imported assets. These
   checks materially support executable independence; screenshots alone do not.
   [HANDOFF integration entries and their `production-closure` reports.]
5. **The agent continued after gates and commits.** It repeatedly stated the next
   runtime boundary and left the complete goal active. The original instruction
   successfully overrode the repository's one-step inner loop. [S:3504,6399,13100,
   16313,20787; GOAL §0 still contains the shorter inner-loop wording.]

## Where steering should change

### Composition is now the limiting queue; add integration capacity before more leaves

The project no longer lacks workers. MainMenu, fresh-save message/frame/writer,
queue, texture backend, CharacterSelect preview, camera and actor recovery were
available as separate private packets awaiting canonical-owner/runtime integration.
A completed packet can be useful, but its private oracle count is not product
completion. It becomes risky when another packet is started without an identified
caller/interface or when snapshots become stale before landing.

Concrete manifests inspected:

- `/private/tmp/sadx-native-main-character-stage/core-main-menu-packet/packet.json`
- `/private/tmp/sadx-save-frame-stage/final-packet.json`
- `/private/tmp/sadx-saving-message-stage/frozen-saving-message-packet.json`
- `/private/tmp/sadx-message-queue-stage/final-packet.json`
- `/private/tmp/sadx-native-character-select-stage/frozen-preview-packet-v2/packet.json`
- `/private/tmp/sadx-native-selection-camera-stage/frozen-camera-packet/packet.json`
- `/private/tmp/sadx-native-sonic-selection-actor-stage/frozen-sonic-initialization-packet/packet.json`

The last three explicitly disclaim a native CharacterSelect renderer, scheduler
integration or complete Sonic factory. The Sonic packet covers the seeded
post-`PInitialize` continuation and destructor, with unresolved animation payloads
and actual collision/texture/motion services. The next bounded work should close
those connected callers. It must not replace them with a static pose or count
resource import as an actor implementation. [Packet `source_scope`, `limits`,
`next_actual_source_boundary`; HANDOFF MainMenu.]

Recommended operating rule: retain a short current dependency list identifying the
next visible checkpoint, its first actual unimplemented call, canonical owner,
ready packet and exact acceptance check. Workers should remove independent items
from that list; the root should consume completed packets continuously. Reconcile
existing packets before asking workers to recover the same function again. This
is a scheduling recommendation, not a new fidelity exception.

### Independent evidence must include ownership and composition, not just case volume

The strongest demonstrated verifier weakness is the quick-open dialog mapping.
The native implementation and its function probe/oracle both mapped the reset to
fade instead of the animation counter. Thousands of local cases could agree on
that shared mistake. Full native/reference images caught it; a separate evidenced
judge correction retained previous rows and added discriminating values. Five
complete 640×480 frames then matched at aligned input delivery. [S:20240;
`b196129`; HANDOFF MainMenu.]

Earlier examples have the same shape: draw-disable/pending-logic counter versus
actual loop count, saved-story flags versus event workspace, and active versus
fallback filename. These are integration contracts, not arithmetic failures.
[HANDOFF FileSelect controls, FileSelect and dialog entries.]

For each system, compare source addresses/types once at the adapter boundary,
then verify distinct values for aliased-looking owners, callback order, failure
state publication and the actual composed frame. Keep images plus semantic state;
matching a crop or aligned timer does not establish full-frame timing or gameplay
parity. Do not call a zero oracle exit a strict pass when its verdict retains FP
partials. [HANDOFF title/FileSelect/MainMenu and STATUS oracle limits.]

### Verification scheduling and build ownership still have preventable friction

Observed incidents include stale scalar/header fingerprints (Oct20 10:26), stale
Input reports (12:27), four observer-fix isolated gates missing prerequisites,
a duplicate audio unit-test target (Oct21 09:47), an export missing Make's generator
prerequisites, a long windowed smoke-test stall, a missing persistent scaling owner
in the logo test and a stale object from a worker draft. These have different
causes; there is no basis to attribute all elapsed gaps to the model being idle.
[S:3075,5775,18408,22200,22486,22662; HANDOFF observer/dialog.]

Preflight clean-build inputs, target registration, profile/judge source identities
and affected probe freshness before the full gate. Keep one owner per shared build
and use genuinely isolated outputs for worker verification. Run the full gate at
a coherent connected checkpoint; retain focused checks while implementing. When
stalled, capture the failing process/command rather than silently rerunning or
relabeling failure. None of this requires weakening or skipping a required judge.

### Keep actual policy separate from an agent's preferred implementation

A current worker packet included source-proved 8×132-byte player parameter content.
A subsequent worker memory update, visible in its own tool call at Oct21 15:51:35,
called compiled literals mandatory under “STYLE §5” and treated parameter metadata
as reference-only. That absolute attribution is incorrect: STYLE §5 describes
unrecovered behavior seams; STYLE §8 and ADR-0007 §5 explicitly permit typed physics
content tables in the imported pack. [W:12663;
STYLE §8](STYLE.md),
[ADR-0007 §5](decisions/ADR-0007-readable-game-code-and-extensibility.md).

A compiled typed table might still be an acceptable implementation choice. The
specific raw-word JSON may need semantic decoding. Neither observation establishes
that imported physics content is prohibited. Resolve classification from evidence
and the actual documents instead of inventing a stricter global rule and giving
workers contradictory requirements. Do not weaken runtime executable independence:
reading a typed imported pack differs from reading `sonic.exe` during play.

### Refresh the progress model without rewriting the acceptance target

At the pinned checkpoint STATUS's narrative reports required native menu journeys,
but its visible-journey section still says native N1–N5 are unverified by the gate.
The generated milestone remains M1 hybrid headless, while the user goal is a
connected native route. This makes otherwise careful evidence difficult to read
and encourages future sessions to repeat old audits. Update the generator/schema
and current native architecture descriptions as a focused documentation/evidence
change; do not hand-edit generated facts. [STATUS](STATUS.md), [GOAL](GOAL.md).

A useful ongoing report has three separate columns: latest committed native
checkpoint, verified but uncommitted native work, and reference-only progress.
Beside them list the next concrete missing call and acceptance failures relevant
to that path. Do not substitute total gate/probe counts for this report, or attempt
to clear every unrelated historical oracle failure before reaching gameplay.

## Steering priorities supported by this investigation

1. Preserve the current branch/worktree and consume the existing reviewed packets;
   do not restart the audit or port.
2. Finish coherent fresh-save integration verification and persistence, then close
   actual CharacterSelect resource/camera/actor lifecycle and normal Sonic entry.
3. Advance native Chaos 0 and the connected route using the reference replay as a
   judge; keep reference and native proof separate.
4. Rank recovery by real runtime dependencies and keep the main agent integrating.
   Avoid a growing backlog of exact but unbound leaf modules.
5. Preserve protected journeys, independent reviews, canonical ownership,
   source-only builds and honest partial verdicts. Strengthen composition contracts
   where matching local oracles demonstrably missed wrong owners.
6. Measure 63 FPS only during native gameplay. Preserve existing hardware evidence
   and user mixer preferences; request new physical interaction only when a changed
   hardware path needs it. User volume changes were an explicit side request,
   not permission to change faithful source audio scheduling. [S:17588–18018;
   HANDOFF dialog.]
7. Keep pursuing the full route after a checkpoint or commit. The evidence supports
   better integration scheduling and clearer proof, not a broader redesign or an
   easier definition of done.
