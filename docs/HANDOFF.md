# Handoff journal

The per-step journal of sadx-macos. **Start with [`docs/GOAL.md`](GOAL.md)**, which holds the
brief, tracks, ordered items and loop. Coding rules are in [`docs/STYLE.md`](STYLE.md) and tools
in [`docs/METHOD.md`](METHOD.md) §0. Append new entries **at the end**. Everything from the
next heading down to the 2026-10-12 sections is the historical record of the `rea` repository
era. It mentions Hopper and `SADX_AGENT_GUIDE.md`, which are retired; read it as history.

New entries are short (at most 15 lines), one per step, in plain words with spaces between
numbers and words. Since 2026-10-16 (ADR-0008) a step has one entry, not a "source" entry plus a
"verification" entry. Use this template:

```markdown
### <date> — <item from GOAL §6> (track J/R/S/V/E, gate <n> or "no gate")

* Outcome: what the user can now see (journey row + `make` command), or the debt removed
  (file, ratchet counts before → after), or what this unblocks.
* Changed: the system, functions and state in game-tier terms, plus the adapter.
* Verified: the level GOAL §7 requires for this kind of change, with counts; gate name and rc.
* Not run / unverified: skipped tests, scope limits, assumptions added.
* Next: the item a fresh agent should take.
```

Entries before 2026-10-19 are preserved in [the historical journal](history/HANDOFF-2026-10-10-to-18.md).

### 2026-10-19 — readable initialization and controlled Chaos 0 (J/R/S, gate 176)

* Outcome: fresh menus reach Chaos 0; keyboard, pad and jump pass with zero rejected draws across 3,005 frames, using explicit original-task reference mode. H/G states and images match at all 14 checkpoints.
* The quoted library initializer is now named configuration, state and services; its four alleged rectangle words are company/application/system/backup strings. Scene dispatcher is also rewritten, not relocated.
* Library: unchanged 2,400 cases / 72 outcomes / zero mismatches; 36 native callback/failure-prefix runs; three faults caught. Scene: 1,632 / zero values or boundaries / 1,344 FP flags; helpers 8,076 / zero values / 6,040 flags (partials).
* Scene old/new: 11,424 development + 10,752 holdout runs identical; three faults caught (horizontal offset needs rotated holdouts). Game-tier totals remove 479 hex literals, 189 address names and 11 address calls.
* Logo update: 5,320 pass; display: 3,936 / zero values / 1,902 flags (partial). Assets-only native visual is a direct-entry fixture, not N1 or pixel parity. New line-list CPU/GPU checks and deliberate topology fault pass.
* CompareStringA unblocks new-save creation; camera/profile telemetry, isolated overlays and prompt failure reporting support unattended checks. Station Square passes movement/jump/pad/camera checks.
* Gate 176 exit 0: 170 native programs / 70 Python tests; R1–R4 pass. First gate exit 2 was a remaining legacy test include/link; assertions unchanged. Temporary task tracing was removed before the final build.
* Limits: Chaos run is guest-reference, 9.71 host FPS with configured 63; no boss defeat, Emerald Coast goal, audio/Windows parity or native boot. Full standalone oracle suite and separate make m1 skipped; nine known oracle failures remain.
* Next: qualify the pinned Input mod CRT RumbleTimer ownership bridge, then rerun Chaos with the default native task adapter. Research and bounded evidence: RESEARCH-breakthrough-2026-10-19.md and production-integration-20261019.

### 2026-10-19 — default task ownership through Chaos 0 (J, gate 177)

* Outcome: fresh menus reach controllable Sonic against Chaos 0 with the default native task adapter. Nine checks pass; all 14 states and PNG hashes exactly match the explicit original-task reference.
* Fixed: the pinned Input mod owns a CRT RumbleTimer in task +0x28, not arena force work. Its scoped adapter sidecar preserves the pointer and requires the real destructor to clear it before arena cleanup; unknown external work still stops.
* Actual-DLL test covers creation, six dispatches, deferred deletion, CRT free, complete arena bytes, slot reuse and repeated pool initialization. Three ownership negatives and all 12 existing task-corpus mutants are caught.
* Full 3,005-frame Chaos route has zero stops/rejected draws; keyboard/pad actions are locomotion, jump is airborne (damage cannot pass). Live CCEF/EHCF/Skipma/63 FPS configuration is checked.
* Gate 177 exit 0: 171 native programs / 70 Python tests; R1–R4 pass. Report source/artifact identities and gameplay evidence are under production-integration-20261019.
* Limits: guest-reference game, not native product; measured 4.72 host FPS during the concurrent gate, not real-time 63 FPS. No boss defeat, Emerald Coast goal, audio or Windows parity claim. Full standalone oracles and separate make m1 skipped; nine prior oracle failures remain.
* Next: complete a native session closure and profile guest/task-transfer costs; continue whole-module readability work under unchanged judges. Do not regress into address-call wrappers or per-constant steps.

### 2026-10-20 — native Start/logo session and platform delivery (N1/R, gate 178)

* Native app now executes Start → LogoInit → both logos/fades → Movie from imported resources; all 16 checkpoints through frame 388 match reference state and complete image bytes. SEGA/Sonic Team images inspected. Movie/title/story remain open; this is not the full requested route.
* Recovered bootstrap, profile save defaults/CRC, PVR drawing, camera and immediate device composition have actual session callers. Native target links no guest/loader/Unicorn or original DLL; imported action graph owns source-exact Sonic entries 100/101. Packed16 texture normalization stays float until GPU sampling.
* Startup 1,338, save 1,029, bootstrap 418 and game-mode 24,000 cases match; holdouts 1,362/1,053/1,186 match. PVR 3,957 and depth 4,000 have zero values mismatched but 1,932/3,879 FP flags (partials); native view 3,000 primary + 3,000 seeded matches unchanged judge. Startup/save/bootstrap/PVR/view faults caught; fresh independent reviews resolved store ordering, unsigned dimensions and shared menu-owner alias.
* Physical Switch 2 Pro USB buttons/sticks/triggers/reconnect qualify through pinned native SDL3/libusb; user confirmed rumble. Exact raw-axis conversion, virtual SDL polling/reconnect and HLE motor delivery pass. Physical captures precede later polling/build revisions; those revisions have automated coverage.
* Native AudioQueue delivered effect 90,112 frames and music/voice 135,168 frames with zero device errors; platform output only, not recovered game sound scheduling. Tests cover callback-safe failure cleanup.
* Reference normal inputs defeated Chaos 0 and entered/played Emerald Coast. Inactive source DestroyTask no-op fixes death retry; boss telemetry is read-only. Replay files preserve inputs; new extended replay determinism and Emerald Coast goal remain open. Later train/boss/stone/Windy Valley unverified.
* Gate first attempt exit 2: new service declarations changed a fingerprinted scalar-state header. Moved services to their own header; original state header and its unchanged fingerprint check now pass. Gate 178 exit 0: 182 native programs, 81 Python tests and R1–R4 pass. STATUS refreshed. Full standalone oracles/m1 skipped; aggregate verdict still exit 2 with nine existing failures.
* Next: integrate staged native movie decoder (full real intro completed), recovered movie control/fade, immediate Sprite2D, then real title tasks/resources. Isolated stages: /private/tmp/sadx-movie-stage, /private/tmp/sadx-movie-game-stage, /private/tmp/sadx-sprite-stage. Preserve worker ownership; no game assets/binaries committed, no push.

### 2026-10-20 — native movie build preparation and startup reports (judge, gate 179)

* Refreshed seven verified startup oracle summaries after source commit 640522f; game-mode source identities now include the separated native service header. Existing cases/comparisons unchanged; 24,000 game-mode cases still match.
* Declared Apple movie frameworks, qualification target and next native movie source closure. New movie sources remain staged outside the repository until this judge commit; no movie/title implementation claim from this step.
* Gate 179 exit 0: 182 native programs, 81 Python tests, R1–R4 pass. STATUS refreshed; PVR/depth partials and nine existing failures retained. Next: integrate staged movie/platform/immediate-rendering callers and native input, then post-intro cleanup/title. No push.

### 2026-10-20 — native intro, input and immediate rendering (integration; gate pending)

* Real native Start/logos now enter the original movie loop. Normal Enter input skips it;
  a separate full intro decodes 3,233 frames and submits 4,761,600 audio frames with
  9,269,451 nonzero samples, 4,647 completed AudioQueue buffers and zero device errors.
  Both release movie textures/media and exit 1 at explicit `clear_game`; title/story remain open.
  Full-run sampled peak RSS 214,320 KiB, ending 64,320 KiB. Images inspected; no gameplay FPS claim.
* Native Input runs pinned key/config normalization, OnInput, GetControllers and soft reset;
  service callbacks consume actual session mode and input. Card result is a separate owner
  from save phase. Native sprite UVs, clipping cache, retained vertices, fades and nested
  frame counters preserve source ordering. Source movie padding is undefined; native clears it.
* Startup still matches all 16 complete images/states. Movie control 756 and fade 2,936
  match; geometry 768/192 flags, timing 688/652 flags, sprite 1,927/1,920 flags and clip
  3,000/1,251 flags have zero value mismatches but remain partial. Native Input covers
  3,292 polls, 2,019 config cases, 567 composed frames and 256 reset cases with zero value
  mismatches and explicit FP partials. Game-mode 24,000 and bootstrap 418 still match.
  Six movie, six rendering and ten input mutants detected; fresh reviews resolved input
  casts/DTO boundaries and found no remaining confirmed integration defect.
* Independent clean ARM64 build with nonexistent original/Unicorn paths exits 0; no
  loader/guest/HLE/Unicorn linkage or original PE files in its build tree. That executable
  displays SEGA (exit 0), then plays/skips the intro to the same `clear_game` stop (exit 1).
  Evidence: `production-integration-20261020/native-movie-{integration,production-closure}.json`.
* Full gate attempt exits 2 on stale tracked Input source-data fingerprints, not an altered
  comparison. Raw initialization/constants checks are rerun; source-first commit followed
  by a separate report/judge refresh is required by JUDGE.md. No successful gate recorded
  for this source step yet. Existing nine oracle failures remain; full oracle sweep/m1 skipped.
* Next: finish the report-refresh gate, then integrate staged Clear/Reset, save loading and
  audio ownership with native screen setup. Private stages: `sadx-cleanup-stage`,
  `sadx-audio-cleanup-stage`, `sadx-game-view-stage` under `/private/tmp`. Screen setup
  matches the unchanged 500-case judge (291 FP partials); it is not integrated yet.
  Reference worker continues Emerald Coast from normal retries. Full persistent goal active;
  no assets/binaries committed, no push.

### 2026-10-20 — native movie report refresh (judge, gate 180)

* Source commit ba931d7 is now covered by gate 180: actual exit 0, 186 native programs,
  92 Python tests and R1–R4 reference journeys pass. Native intro/skip evidence remains
  separate and still ends at `clear_game`; no native title/story completion claim.
* Refreshed 12 source-verified reports. The coefficient isolation checker also needed the
  newly separated `engine/controller_values.hpp` in its copied/hash-checked include closure:
  before exit 1 on undeclared include; after pass, unchanged seven source words match.
  Input initialization still compares all 1,869 bytes. Eleven judge regression tests pass.
* Aggregate oracle verdict exits 2; existing nine failures and all FP partials remain
  explicit. Full standalone oracle sweep/m1 skipped. No tolerances, cases or expected values
  were changed. Next: integrate staged cleanup/save/audio/queue/screen owners and continue
  through title initialization. No push.

### 2026-10-20 — Bound reference observer draw history (track E, full gate exit 0)

* Outcome: preserved the live normal-input Emerald Coast route while reducing observer footprint from 9.2 GB to 1.7 GB.
* Changed: interactive reference sessions disable legacy whole-session draw capture; current frame rendering and default differential capture remain intact.
* Verified: regression fails before the fix (2,001 retained draws, exit 134), passes after it (one explicitly captured draw, 2,001 presents); graphics frame regression passes. Fresh independent review found no blocker.
* Live recovery: retired 2,761,642 historical draws using a separately reviewed development-only helper; same-frame state and PNG remained byte identical; 30 normal frames later history was empty and footprint was 1.7 GB.
* Gate: make -j10 gate with pinned private CMake exited 0; 187 native programs, 92 Python tests, four reference journeys passed. Four earlier isolated-checkout attempts exited 2 because tool/asset/corpus inputs were absent; complete input preflight and exact fixture reruns resolved them.
* Limits: no gameplay performance claim; native title WIP was excluded from this isolated gate. Native connected story remains incomplete.
* Next: integrate native title input and actual music playback, then continue the normal-input story route.

## 2026-10-20 — Native title dependency integration in progress (no new gate yet)

The real assets-only application follows normal Enter at frame500 through Movie,
Clear, Reset and save preparation into TitleInit. Latest completed run
`build/native-title-lighting-journey.log` exits1 at
`native_regular_object_texture_loading`, after the real title initializer loaded
four effective-profile sound banks (91 decoded samples,11 open attempts) and ran
native stage lighting and camera setup. Its movie delivered78848 frames,
94266 nonzero sample values,74 completed buffers,error0. The inspected frame501
is black; this is NOT visible Title/N2 or native gameplay. Earlier bank-only run
stopped at stage lighting. No new full gate has been run; gate180 remains last pass.

Current source is uncommitted integration, with independent owners active. Root
owns App/Session/Display/TitleScene composition and shared builds. `native_title`
owns title core/factories/probes/imported menu data; `native_logo` finished task
rendering+lighting and now owns effective regular texture wrappers/importer;
`story_route` owns the durable reference continuation. Do not overwrite their files.
Task/display and lighting handoffs: `/private/tmp/sadx-task-display-handoff.txt`,
`/private/tmp/sadx-stage-lighting-handoff.txt`. Fresh Sol6.1 medium review
`review_title_dependencies` found no defect in task/lighting/camera and independently
compared all36960 imported/native lighting bytes with the original image.

Task display:236 dev/556 holdout zero mismatches,6 mutants killed; old lifecycle19
and work16 sequence judges pass unchanged. Lighting:704 dev/1408 holdout zero,
7 mutants killed. Camera SetViewAngle:2379 dev/2379 holdout zero,4 mutants killed
(`/private/tmp/sadx-camera-view-mutants/summary.json`). Frame counter scalar source
and native code were added by title worker; root wires outer-loop start before
input and completion scalars before presentation. Nested movie WaitVSync is a
separate entry and does not perform those scalar stores.

Important recovered facts: stage lighting table entry255 starts zero; first source
sentinel is174, and InitDefaultStageLight writes255's sentinel later. Palette255
already contains FF. Cocoa decimal parsing can change the intermediate binary64
number, but native binary32 conversion matches all source bits (unit
`native_lighting_assets`, source hashes from importer). SetViewAngle437240 is NOT
just njSetPerspective402ed0; it has signed camera-type28..30 interpolation and a
post-call owner reload. Effective language comes from the selected loader JSON
profile, not sonicDX.ini. Current imported selection gives text1/voice1.

Regular texture dependency must implement active loader LoadPVM_r at421180 and
njLoadTexture_Wrapper_r at4228e0. The original wrapper is not the production rule.
The loader can expand a target list or repeat its final loaded entry; no strict
archive-count equality. It sets canonical LoadingFile/resync before backend load.
Root exposed LogoRenderer::load_pvm_archive as a raw asset backend only; worker
owns the semantic wrapper. Apply the same wrapper to the original logo caller
when ready, preserving the previously exact16 screenshots.

Build preparation commits since gate180:1e14116,3231ca2,479dd27,7c96922,773ddcc.
They are explicit native dependency/judge preparations, not additional gate passes.
The camera build preparation first used a wrong unittest pattern (0 tests); the
correct test_judge.py was subsequently run and all11 passed. New test rows and
one explicit native screen probe rule are being added with the additions-only
judge procedure. Original judges/tolerances/cases remain unchanged.

### 2026-10-20 — native title constructors, task callbacks and caller composition (worker)

* Actual fresh-title sequence now has all thirteen native factories, typed auxiliary work and
  real scheduler callbacks in native_title_scene/native_title_tasks. Source inactive exec/display
  guards preserve CharacterSelect aliases, unconditional option animations and Puti first-exec
  MAlloc/callback stores; active unrecovered branches fail explicitly. Dialog preserves genuinely
  indeterminate allocation counters with semantic optionals. Mode-resource caller506010 and title
  fade/color helpers are composed; next resource stop is actual menu texture ID18 acquisition.
* Standalone repository C++17/Werror builds and unit pass;15 importer tests pass. Original-instruction
  dev/holdout: factories2268/2816 PASS; inactive exec/display/Puti3753/4301 PASS. Main21372/21920,
  dialog2652/3200 and resource/color3810/4358 compare every value exactly but remain FP partials
  (271/282,667/753 and1277/1514 flag differences respectively). Prior32 mutations and18 new
  mutations caught, plus earlier three frame-counter mutations. Standard frame-counter CLI2004 PASS.
* Engine DeleteAllObjects40b730 preserves ascending list order, exec-null survivors and recycled
  saved-next slots:244/305 compared cases match;6/9 individually listed reentrant-destructor/source
  free-list paths remain unverified. Unchanged display236 and lifecycle19-sequence judges pass.
  Readability/global owners pass; profile check0UNHANDLED/0undetermined. No shared make/full gate,
  live title image, independent review or complete title/story claim from this worker. Parent owns
  final integration/review. Evidence: ignored native-menu-family/native-menu-callback/title-support
  dossiers; private /private/tmp/sadx-title-menu-stage/*-final-{dev,holdout}.json and mutation reports.
  No original assets/binaries committed; no push.

### 2026-10-20 — title review fix: Puti task-owned work

* Fresh review found that a second Puti task reused scene-cached work and skipped its own
  auxiliary allocation/callback stores. Removed that cache; source update_puti now reads and
  publishes only the current task's typed auxiliary work. Scene passes the task directly.
* Regression runs two actual source-pool tasks through scheduler callbacks while retaining and
  poisoning the first work: two distinct16-byte allocations and callback installations, then
  both tasks reuse their own work. Unit and strict C++17 runtime compilation pass; unchanged
  inactive original oracle3753/4301 dev/holdout passes. A stale-cache mutant fails the new
  repeat-task assertion. Readability0 regressions. Reviewer recheck pending; background unstarted.

### 2026-10-20 — native title background and scaled PVR caller (worker)

* Full original50e470 background now draws clouds/water, sliding logo, lettering and blinking
  prompt through native433f80 scaled PVR quads. Scene binds canonical menu texture lists,
  material/color state and Display scale/tick owners. Importer preserves nine layout floats
  with inverse-checked binary32 words. Original-PC work+34 is now float cloud_offset; the
  existing title judge retains its raw words. Corrected runtime adva counter binding to
  Session.title_logic_ticks_3b11184; cleanup reset_work_3b11190 remains a separate owner.
* Private strict C++17 builds/unit and17 importer tests pass. Original-instruction dev/holdout
  compare every value exactly: background2125/2673, scaled3291/3839, composed background→real
  color/material→scaled vertices437/649, unchanged title21372/21920, unchanged PVR3957.
  These are FP partials, not full passes: respective flag differences1112/1444,2764/3184,
  437/649,270/282 and1932. Composed tests include inherited colors and odd texture dimensions;
  texture lookup/final submission remain receipt seams, with no GPU/pixel-image claim.
* All15 background/scaled/base mutations and prior32 title mutations are caught. Source
  evidence: native-title-pvr-scaled-texture dossier ev_32f5e382a25fc601720bdcf85a82d272de240aaa12076e96b1db5c93167b218a;
  full background PEImage continuation through51011c, since Ghidra falsely truncates at the
  texture setter. Reports/mutants live in /private/tmp/sadx-title-menu-stage. Parent owns
  profile registry update for GVR43434a byte38→00 (native alpha1), symbol integration, fresh
  review, full gate and actual title image. No shared make/gate or live image claimed here;
  source LoadLevelSounds and then Title input/music remain required connected boundaries.

### 2026-10-20 — connected native title input and transition (worker)

* Actual title control50e300 now initializes timing/save-menu state, starts music once per
  task, waits150ticks, checks Escape/Start and follows signed demo timeout/music-state rules.
  Original-PC work+2c is a scalar music_started sentinel, replacing the misleading camera
  handle; existing probe raw inputs/results are preserved. Scene borrows canonical save,
  exit, input, texture-ready and configured-audio owners. Real music request/manager tasks
  compose the other worker's music core, with source levels/callbacks/CAlloc8 and canonical
  media state. Voice-file playback remains an explicit boundary when actually requested.
* Mode transition505b40 stops menu music before setting sequence.waiting=1 and waiting_mode.
  An initial semantic-field mismatch (fade_out versus waiting) was caught during composition
  inspection and fixed; new original comparison also executes actual sequence505c20 to prove
  its nine-ticks-remaining state after the first update. Per-task unit uses two actual source
  pool handles and verifies one music request each, with sentinel published before the call.
* New original-instruction dev5589/holdout6137 PASS;17 new input/transition mutations and
  prior32 title mutations caught. Strict private C++17/Werror unit/runtime compilation pass;
  unchanged old title judge21372/21920 values match (FP flags remain partial). Profile0
  UNHANDLED/0undetermined, readability0 regressions, layers0 violations, owners0duplicates.
  Evidence: native-menu-callback-50e300 ev_5ed243f8385424e716e14920a702d73eefe6de9ab3eb219356ba34515ee45d13;
  native-title-common-mode-procedure ev_570d8ddd20295f092b909dc8a3ac3ea6e732838ab0e4bc80b3f51e67234fb07d.
  Private reports: /private/tmp/sadx-title-menu-stage/title-input-{dev,holdout}.json and
  title-{input,transition}-mutants.json. Next Start-specific boundary505810 is confirmed
  dsPlay_oneshot423d70(2,0,0,0), still explicit native_title_confirm_sound. No shared build,
  full gate or live image claim from this worker; parent owns fresh review and actual run.

### 2026-10-20 — native task owner cleared inside destructor

* Actual Start transition crashed in TaskScheduler::destroy_task after the music destructor
  cleared the canonical TaskHandle passed by reference. The source passes its task pointer by
  value. Scheduler now pins that pointer value at entry, covering every caller without a
  music-only workaround. New task_callback_lifetime regression clears or replaces the same
  caller owner, checks four arena-work releases and auxiliary-native release order: old source
  rc139, fixed source rc0. Unchanged lifecycle19-sequence/display236/delete244 original judges
  match; existing source-reentrant exclusions remain. Parent owns actual rebuilt Start run.
* New title-input reentry vectors mutate live buttons/menu-ready/BGM during keyboard/readiness
  callbacks: original5877/6425 dev/holdout PASS, three cached-owner mutants caught. Existing
  vectors/expectations retained; no existing registered judge edited. Prior title FP dev count
  corrected to270 in the background handoff. Sound server/confirmation recovery continues.

### 2026-10-20 — connected confirmation sound and native sound server (worker)

* Title confirmation505810 now queues actual dsPlay_oneshot423d70 through native channel
  search423d10. New semantic sound_effects module consumes canonical SoundInitialState,
  bank identities and positional-task owners through real dsSoundServer4250d0. Includes
  exact overlap/boss-bank/sample-name selection, record expiry, parameter conversion and
  explicit resource/3D/distance/voice callees. Existing address-shaped sound_manager reference
  remains unchanged readability debt. Parent binds the real LoadPCM→volume→Play backend;
  load does not start playback. New imported bank thresholds and raw-WAV loop metadata are
  typed data, with source-byte/hash/inverse and new-record zero-fill validation.
* Private strict C++17/Werror builds/unit pass; importer11 tests and actual importer→Cocoa
  roundtrip pass. Original dev/holdout: requests2580/3128 PASS; server2084/2468 PASS (real
  overlap, boss-bank, sample lookup and composed confirmation entry). Volume/pan/pitch2440/
  3488 every value matches but x87 flags remain partial553/920. WAV parser506 PASS includes
  203 actual imported payloads; three policy mutations and fifteen queue/server/parameter
  mutations caught. Unchanged legacy sound-manager956, lifecycle19 sequences, display236
  and delete244 compare exactly, retaining previously documented reentrant exclusions.
* Scope: final PCM, distance, 3D/positional and voice operations in the server judge are traced
  resource receipts. LoadPCM's source guard/order matters: initialized→parse/validate nonzero
  data→close parser→resolve loopflag2 from sampler metadata→publish loop mode→StopPCM→create
  buffer, then the caller sets volume and PlayPCM starts it. Failed parsing must not invent a
  replacement buffer. Source volume0 maps−1280 hundredths dB; pitch is integer Hz despite SDK
  float prototype. Original WAV endpoints left uninitialized remain JSONnull when loopcount0.
* Parent's rebuilt actual Start run no longer crashes, loads/plays one real SFX with nonzero
  output/deviceerror0 and reaches next explicit native_file_select_initialization at frame759.
  This worker did not run a shared build/full gate or claim Windows/native PCM equivalence.
  Fresh SFX/platform review pending. Evidence native-title-one-shot-sound ev_c4824c5ceaff1a9d576aec5168f28de07ed1fefd6030677d1dc8f3e16c9c2d49;
  native-title-sound-server ev_e8ee66ebba02a616ae72d1d63e2a9be0c54b890f4452b0f452ab77854a6a7400;
  native-sfx-wave-read-format ev_cf81429602776b261e8442439b49b0eb5c3291ec2ae13e9d539fea26baa4a1ba.
  Reports/mutants: /private/tmp/sadx-title-menu-stage/sound-*.json. Profile0UNHANDLED/
  0undetermined; readability0 regressions/layers0 violations. Parent owns423d10 symbol override,
  new judge/unit registration, final integration/review and live journey reporting. No push.

### 2026-10-20 — Native title and audible Start transition (track J/R/E, full gate exit 0)

* Outcome: the assets-only native app now runs Start → logos → real movie/normal Enter skip → full Sonic title, plays imported title music, and accepts normal Enter with real confirmation SFX. It stops at `native_file_select_initialization` (frame 759); no native gameplay claim.
* Connected: canonical cleanup/texture lifetimes, regular/menu resources, title tasks/background/input, task display/camera/lighting, frame counters, level banks, music and the per-frame sound server. A destructor clearing its own caller handle exposed a real crash; scheduler entry now retains the task, with old-source red exit 139 / fixed green regression.
* Images/state: 16 startup checkpoints exactly match reference pixels and state; the full title matches every pixel at global timer 240 (native 740/reference 742). The two-present movie-skip offset remains an explicit limit. Final snapshots report 432 live texture slots and released movie resources.
* Audio: actual title music and confirmation buffers delivered nonzero samples without device errors. Full imported 579,584-frame title stream also passed mute/resume, EOF and restart; PCM load-without-play, failure preservation, disabled guard and source timer expiry were checked. Advanced variable-rate/sampler/3D/voice branches still stop explicitly.
* Verified: `make -j10 gate build/sadx-native NATIVE_CONTROLLER_CMAKE=…` exited 0: 211 native programs, 172 Python tests, four reference journeys. Thirty-six targeted oracle commands all matched compared values; three exited 1 for recorded FP flags. FP partials and wider existing failures are unresolved, not promoted to pass. Fresh independent reviews covered the recovered clusters and runtime sound/task ownership.
* Production separation: a clean source-only checkout built without original EXE/DLLs or prebuilt game objects, then ran against imported assets and reproduced the exact title. First isolated build exited 2 for absent CMake; supplied pinned native CMake resolved it. See `native-title-production-closure.json`, `native-title-connected.json`, `native-title-oracle-exits.json` and `native-title-integration-gate.json`.
* Memory: reference observer remains 1.8 GB after 1,027 more normal frames following the 9.2 GB diagnostic-history recovery; the connected Emerald Coast replay survives. Reference progress remains distinct from native.
* Next: integrate reviewed private menu resource cleanup; native file-select initializer/save-list codec and menu voice recovery are staged privately. Preserve the canonical `list_data_` owner during its typed migration; use real save enumeration and prefix reads. Native Chaos 0 through Windy Valley, relevant voice playback, gameplay 63 FPS and full acceptance remain open.

### 2026-10-20 — Native FileSelect initialization, save resources and task phases (private proof; integration pending)

* Outcome: new readable file_select/save_file_list/menu_voice/font_native modules are landed for the real frame759 caller. Parent owns runtime composition and gate; no new visible FileSelect or gameplay claim. Full5053f0 phase dispatch now invokes actual initialization/fade/release seams; active503600 controls and display remain explicit next boundaries.
* Fidelity: original505320 init2364dev/2964holdout;505050 discovery348/604;421de0 selected load368/752;voice4888/5788; font load/copy/release1368/1968;504ed0 reverse-tail free101cases. Zero compared mismatches. Full5053f0 update2636/3436 values exact but1119/1463 FP-flag partials (oracle exit1), not pass. Twenty-three targeted mutants detected; save codec2048random inverse cases plus two actual saved files; repeat voice-task unit and five importer tests pass. Strict C++17/private builds only, no shared gate run.
* Source: count preserves provider order and actual FILETIME; partial fread publishes live bytes before filtering. First MAlloc list sentinel retains unknown name/time. Selected save reads BYTE3b290d8 (MOVZX DI), distinct menu.selected_file; fallback borrows bootstrap.save_name. Free walks tail-first, excludes sentinel filename, then clears canonical list. Importers emit pointer-free ASCII metadata and Sonic nine-request voice row; unsupported rows fail explicitly. Profile0unhandled/0undetermined overlaps; readability0 regressions/layers0 violations/global owners0 duplicates.
* Evidence: inventory-raw native-file-select-{initialize,find-save-files,load-selected-save,menu-voice-*,font-*}; validfull5053f0 minimal dossier ev_f24b3fae4b307b072810447b3cca9381161aa230a542e9bfec86362f2e4c2024; fontrelease420a30 ev_dab9eb0f6e058cffdee3ae0de1b0188136cb078aae794651b82288f67f02c8dd.504ed0 Ghidra analyze timed out650s; exact original instructions judged directly. Private final fingerprints/reports/mutants: /private/tmp/sadx-file-select-stage/final-proof and *mutants.json. Native_logo owns valid503600/display recovery runner; no concurrent writer from this worker. Fresh review/parent registrations, full gate and actual journey remain required; no push.

### 2026-10-21 — FileSelect normal controls and readable phase arithmetic (integration/review pending)

* Outcome: landed new file_select_controls startup/selection/navigation/cancel paths for503600, reusing native cursor animation, Ninja cosine/color interpolation and existing save/audio seams. Other dialog stages report their actual stage boundary after the source common animation; no full dialog/visible FileSelect claim yet. Parent binds real queue cues1/3 and canonical runtime fields.
* Source proof:2438dev/3538holdout values exact,938/1370FP-flag partials (exit1). All12mutants fail values on the unchanged full dev corpus; the directed-only subset did not catch the previous-mode guard mutant, so the same full dev corpus was used without changing cases. Repeat-frame startup→navigation/load→OptionSelect cancel guard→cancel transition unit passes. Sourcepreviousmode7 is OptionSelect, not TrialActSelect; the first development run caught this incorrect label and code was corrected, without judge changes.
* Readability fix: game fade arithmetic now uses existing CPU fidelity::ExactFloat integer/multiply_double/rounded/compare helpers, preserving the double0.05 operation width and retained comparison. Unchangedphase2636dev/3436holdout values and1119/1463FPpartials remain; samefivephase mutants detected. Machine-code library hash is unchanged. No low-level X87Context arithmetic remains in file_select game code. Readability0 regressions/layers0/global-owner duplicates0; profile0unhandled overlaps.
* Evidence/API: validnative-file-select-minimal-503600 dossier; newfirst-visible/card/displayed-save borrows preserve16-bit wrapping and byte widths. Source3b291b8 and3b291b3 initial bytes both loaderBSS00, with no existing native owner found; parent owns new neutral session fields. Finalproofs and controls-mutants.json remain /private/tmp/sadx-file-select-stage. No shared builds/gate/commit/push. Independent review pending; next source requests5057b0→432db0/432550 dossiers recovered, no dialog implementation started while peer display review proceeds.

* Counter composition correction (2026-10-21): FileSelect phase/control now borrow unsigned secondary_loop_count_3b1118c, signed only at source FILD. ModeState.pending_logic_ticks owns3b1117c render inhibition; the former runtime binding kept fade0 and showed only font. Root corrected it and actual native1100 fullFileSelect was independently inspected; Down/Up changes realsave data, confirm stops explicitly atdialog request. Sameunchangedphase/control dev+holdouts and5+12mutants rerun, values exact with recordedFPpartials. State.elapsed_ticks is3b11180; previous_loop_starting is3b11194; noTitleTiming ownerchange needed. Rootdisplay View.tutorial_variant also corrected fromwrongflow.title_mode3abdcfc toactualflow.tutorial_variant3b2a2fa. Evidence native-file-select-connected.json; broaderfreshintegrationreview pending.

### 2026-10-21 — Connected native FileSelect and delivered menu voice (tracks J/R/E, full gate exit 0)

* Outcome: native boot/title → normal Enter → full FileSelect; Down/Up load the two real saves and return, with option music, menu voice and navigation SFX. Normal confirmation stops at `native_file_select_dialog_request`; no native gameplay claim.
* Connected readable save-list/codec, font, menu voice/playback, texture cleanup, full FileSelect drawing/control and real filesystem/calendar services. First-launch copies use fresh inodes and exclusive publication; existing native saves remain intact. Production executes compiled code and imported assets.
* Critical owner corrections: draw-disable is `timing.pending_logic_ticks` (3b1117c), logic count is `secondary_loop_count_` (3b1118c); display selection borrows card byte D8; variant borrows `flow.tutorial_variant`; persistent story flags are `bootstrap.sequence_variables` (3b18808), not `event_work` (3b18408). Source serialization and an actual gray-icon regression exposed the last mismatch.
* Images: new original-reference panel hash was red by 17,919 channel bytes before that correction, then the entire 322×362 panel matched exactly. Whole-image differences remain explicitly reported (provider order/dates/cursor). Title still matches native 740/reference 742 at timer 240; the two-present movie offset is unresolved. Current PNG inspected at `build/native-file-select-story-flags-fixed/captures/950.png`.
* Native journey `tools/journey/check_native_file_select.py` passes normal inputs, save hashes, stable texture ownership and actual nonzero music/voice/SFX buffers with device errors 0. A source-only export built without original EXE/DLLs or prebuilt game objects; all six captures match the main native build and its source panel check passes.
* Verified: gate exit 0, 222 native programs, 210 Python tests, four reference journeys. Prior attempts exited 2 for missing profile tracking and an obsolete one-clip fixture; separate evidenced judge commits register the already implemented patches and retain/expand music checks (four malformed cases → eight, duplicate-validator mutant caught). Fresh independent review covered integration and corrected array ownership.
* Fidelity: initialization/save/font/voice/cleanup values match their source judges; phase/control/display/primitives retain FP partials. Colored strips match all 884 dev/1,884 holdout normal cases, with five mutants caught; two raw CW119 Unicorn divergences stay FAIL, not partial/pass. Native unmasked status flags remain unqualified. Existing broader oracle failures remain.
* Evidence: `reference/production-integration-20261021/native-file-select-*`; source-only directory `/private/tmp/sadx-native-file-select-closure-aw5tp91j`. General UI scaling and native story/gameplay/63 FPS remain open. Native-module probes and raw reports were run; tracked oracle summaries follow in a separate judge commit.
* Next: integrate independently reviewed dialog core from `/private/tmp/sadx-menu-dialog-stage` only after real Caption ownership/composition. Worker `native_title` owns private work. Apply its small DialogParameters extension, not its old whole title header. Caption timer/buffer need whole legacy-owner migration; borrow Display's existing registered texture-list count (3b0e334), never add another native scalar.
* Reference continues separately: live observer/driver 68216/68215, Emerald Coast act 1 second checkpoint 21052 and tower climb 22402; no train/Egg Hornet/Wind Stone/Windy Valley proof. Its footprint is still 1.8 GB after 6,006 more frames with bounded draw history. The complete session goal remains active; nothing after the requested push of 1c864be has been pushed.

### 2026-10-21 — Native save confirmation, cancel and reentry (full gate exit 0)

* Visible: normal Enter opens the actual OK/Cancel/Delete popup; OK publishes the selected real SaveName and reaches explicit `native_main_menu_initialization`. Cancel → FileSelect → title → Enter → FileSelect reentry is enforced by the protected 1,850-frame journey. Original 1,100-frame assertions and exact source saved-story panel comparison remain. No MainMenu/character-selection/gameplay claim yet.
* Integrated independently reviewed dialog/Caption core, whole Caption and SaveName ownership migrations, one named FileSelect control dispatcher, all 81 menu-voice entries and 162 actual JP/US clips. Active SaveName3b290dc remains distinct from fallback3b290d0; actual confirmation records name02/fallbacknull/restored Sonic voice cursor1. Reentry delivers four music, two voice and nine SFX events, with nonzero device samples and no device errors; 118 real caption allocations/waits/frees balance, with 477 resident texture slots.
* Full gate `build/native-dialog-gate-fixed.log` exited0:228 native programs,232 Python tests(one existing filesystem skip),four reference journeys and required native journey. First attempt exited2 for duplicate new audio test registration; original audio-gain test restored byte-identically and distinct audio-balance target registered in separate evidenced judge commit aeb6e05. Canonical dialog/Caption/thread/owner/save/control/display source judges and holdouts rerun; FP partials stay partial, including a zero-exit display oracle. Six new dispatcher/confirmation mutations all cause value failures; prior 22 core and18 ownership/thread mutations retained.
* Source-only export `/private/tmp/sadx-native-dialog-closure-nplaw0hj` builds and runs the protected journey with exit0, without exported EXE/DLL/game assets or prebuilt game objects. All11 captures match main byte-for-byte. Initial export omitted Make's test-generator prerequisites and exited2 before compilation; corrected export includes source harnesses. Limits and actual statuses are in `reference/production-integration-20261021/native-dialog-{connected,production-closure,integration-gate}.json`.
* User listening balance is optional, live at500ms, with unity defaults and `--original-mix` bypass. Latest feedback lowered local narration3→2 and raised effects15→22%; music remains5%. The fresh source-only application records and delivers these gains. Source attenuation and queue timing are unchanged.
* Next actual caller: integrate reviewed MainMenu core `/private/tmp/sadx-native-main-character-stage/core-main-menu-packet/packet.json`, source style4 shared square helpers, dialog definitions2/3/4/content callbacks, real eligibility/language predicates and music request1→clip101. Fresh save requires reviewed `/private/tmp/sadx-save-frame-stage/final-packet.json`, saving-text/font/upload `/private/tmp/sadx-saving-message-stage/frozen-saving-message-packet.json` and late-sprite/queue `/private/tmp/sadx-message-queue-stage/final-packet.json`; all still private. Preserve canonical save phase=`flow.save_phase`3b291a4, ready3b291ad, pause3b28114; message arena is actual0x88000 queue storage. Active loader404660 requires real UIScale-inactive qualification, no unconditional fallback.
* Source CharacterSelect needs actual DLL action/EXEmotion resources, all seven player factories, event animations and camera; exact dependencies are in the MainMenu bounded-extension packet. No placeholder 3D or manufactured story flags. Observer68216/driver68215 remain alive frozen at reference EC act1 third checkpoint23255,33 rings; reference-only. Native Chaos0→WindyValley, fresh-save persistence and measured gameplay63FPS remain open; persistent goal stays active. Last authorized push was1a8261f; no further push. Preserve unrelated two untracked Ghidra request JSONs.

### 2026-10-21 — Native MainMenu connected, full gate0, freshsave/CharacterSelectnext

* Actual native normal save confirmation now reaches the full MainMenu, with source19/20textures, style4 square dialogs2/3/4, real trial/mission/optional predicates, language getters and music101. NormalDown/Cancel→Title→Enter→MainMenu reentry runs1850frames exit0; allsource callers/resources bound. AdventureEnter1350 reaches explicit `native_character_select_initialization` at1381; nocharacterselect/gameplayclaim.
* SourceRootHEADb196129: separate judge commits67e9dd1(newsourcejudges/build/legacy2musicfixture),52099b6(required+protectedMainMenujourney),b196129(genuinequick-openjudgecorrection). Main implementation is the separatelyverifiedportfollowingthosejudgecommits. Fullgatebuild/native-main-menu-gate.log exited0:236nativeprograms,245Pythontests(oneexistingfilesystemskip),4referencejourneys,requirednativeFileSelect1850+MainMenu1850. Reportnative-main-menu-integration-gate.json recorded; STATUSgenerated.
* Actualintegrationfoundwrongquick-open store: source432d0e clearsangle+32, notfade+20. Oldcontrolprobe/oracle sharedwrongmapping. Separatejudgecorrection retains48wordABI/allold2058rows, addsFade1.5samecohort+knownness; wrongstore/resetmutantsRED, fixed4116dev7716holdout strictPASS. Fivewhole640x480frames1248..1252matchoriginalexactlyafterobservedinputdeliveryalignment(native952/1052 vsreferencepress950/1050); productioninputs/oldjourneysunchanged. v3packet/privatefreshreviewfrozen. Globalmovie2presentoffsetstillseparate.
* Next: source-onlyMainclosurecompleted(build0/journey0/all10capturesexact/noOriginalExe), nowintegratereviewedfreshsaveCreation/Frame/Writer andfullMSGfont/raster/upload/frame. Rootprivatecomposition `/private/tmp/sadx-native-save-integration-stage` hasreviewednewmodules+arena/sprite/CaptionBitmap/signedFOVoverlays; runtimebridgesnotyetwritten. Preserveworkerpackets; noMainSavedFov/texture/Inputresetintegrationyet.
* SavedFovcorrection: BOTH nativeScreen.last_perspective andCameraAngle.last_angle actually3b2cbb4; signedSDKsharedSavedFovState reviewed. RemoveDisplay.last_perspective_whenbinding; current3aad0bc/copy3ab98ecdistinct. Packet `/private/tmp/sadx-saved-fov-stage/final-packet.json` surgicalpatches, unchangedoracles+freshreview; doNOTcopywholeoldreferenceheaders. Backend `/private/tmp/sadx-message-texture-backend-stage/frozen-message-texture-backend-packet.json` reviewed,actualMetal3readbacks; ownsrealpaletteonce/nosecondpool. InputReset40f350packet `/private/tmp/sadx-save-input-reset-stage/final-packet.json` underfreshreview; actualkeyboardrowsstart3b0e3e0, notlegacyshift3e2.
* Activeworkers: story_route(privateCharacterSelectpreview/resourcepack03),native_logo(privateactualUIScaleStack+frame-tailqualification404660),review_save_input_reset. NoMainworkerwrites/sharedbuilds. RootMainfullgatefinished; currentRootsource-onlyclosurebuildprivate. Referenceobserver68216/driver68215remainfrozen23255; neverkill. Useraudio localmix music.05/effects.22/voice2; nohardwareinteractionneeded. No pushes after1a8261f. Fullnativeacceptancegoal remainsactive.

### 2026-10-21 — Reference Emerald Coast completion and actual Mystic Ruins train arrival (reference worker)

* Existing observer68216/driver68215 continued from23255 using988 additional normal-input/observation commands; no guest writes, teleports, flags, save edits, restart or shared builds. EC goal first captured26644/action20, score4:30:50/40rings at26944, emblem award27184, actual Sonic/Tails Station Square return27544, free control27694 with visible Tails-unlocked message. One normal spike-gate hit25644; recovered rings/spindashed through, no deaths in this extension.
* Actual hotel pool→lobby→front street→station stairs→station interior28748→platform→open carriage door29868→closed doors29988→Mystic Ruins arrival30168. Frozen30238:mode5,stage33,act0,control1,idle Sonic(-9,213,1200),neutral keys/stick. Arrival screenshot inspected; read-only expectation passed. Preserve observer/driver and pinned history-recovery helper; no commands pending.
* Full replayable normal prefix2467commands: `reference/story-route-20261020/sonic-story-through-mystic-ruins-train-arrival.recorded.json`, SHA2566a88fd5d93d72580353be7a71b35d210cfc090729a687f3cfe15a7778be88071; paired `.recorded.evidence.json` has source/reference-only qualification, snapshots and state. EC-only2083-command prefix SHA256611060dcb38ae95b70c260db1eb16bcd4c361cc61bdd8d30c93139cd78ad6a96 also frozen. Durable `continuation-handoff.json` and livecheckpoint updated. Extension acquired live, not independently replayed; no native acceptance/Windows parity claim, no gate run or production code changed.
* Memory remains bounded at1.8GB physical footprint/229.7MB allocated heap after13842frames since recovery; read-only vmmap saved under durable-continuation/memory-at-mystic-ruins-arrival-30238.txt. Source SET/PE train assembly used only as navigation evidence. Next unattempted story route: descend Mystic Ruins station toward Eggman/Egg Hornet and Windy Valley.

### 2026-10-21 — Current CharacterSelect integration dependencies (Root, in flight)

Fresh-save native creation/restart and empty-profile01 are independently verified, implementation uncommitted pending its complete integration gate. HEAD e6917c8 contains separate judge preparation only. Existing reference observer/driver68216/68215 preserved at30238; EC/train extension awaits independent clean replay.

| Actual caller/callback | Canonical owner | Remaining binding/resource | Reviewed packet | Owner | Integration check |
|---|---|---|---|---|---|
| CharacterSelectMenu_Main5122d0 → Initialize5114e0 | existing task auxiliary CharacterSelectWork; bootstrap.character WORD16 distinct from menu selected byte | actual update/draw/input/dialog7–10/music3–4 | character-menu-stage/frozen-packet | Root | normal Adventure entry, cancel/reentry, real dialog/audio samples |
| Initialize5114e0 → CharSel_LoadA512bc0 | session preview task; existing texture lists/counters; shared SavedFov | real factory callbacks, actor action512910, one combined imported CharacterPack | character-select-stage/frozen-preview-packet-v2 | Root | reached lifecycle/all seven actors, no pose-only substitutes |
| CharSel_LoadA → palette412420(2), camera433410 | existing lighting tables/stage/time/queue palette; Display matrix/view::Camera | native device-light owner and actual camera task callback | selection-lighting-stage/frozen-packet; selection-camera-stage/frozen-camera-packet | Root | actual callback order, light draw snapshots, actor image |
| real Sonic49a9b0 → CCL_Init41caf0 | TaskWorkArena primary/collision; stable mutable model registry | clean PInitialize/animation table, actual actor common-tail/update/display; CCL packet finishing | sonic-selection-actor-stage/frozen-sonic-initialization-packet; collision worker private stage | story_route: CCL; Root: actor integration | real task cleanup/reentry/contact lifecycle under unchanged judges |
| all player factories and actual rendering callbacks | existing task/motion/player owners | remaining reached animation/model/draw helpers, authored resources | existing preview/Sonic packets; explicit uncovered branches | Root assigns next reached dependency | visible animated actors and source-native images, normal Sonic confirmation/story entry |

Policy correction: STYLE§8/ADR0007§5 permit typed imported physics parameters; the earlier compiled-only restriction was invented and is revoked. Guest adapters remain excluded from production. MainMenu visual and empty-profile regressions are being strengthened privately; native gate reporting now being corrected from actual machine-readable checkpoint summaries.

### 2026-10-21 — Native fresh-save persistence integrated, full gate0

* Actual native normal inputs create03 from populated snapshot and01 from an existing empty profile; restart, load/confirm, cancel and title return/reentry pass. Both files have identical1392bytes/checksum50276; imported/existing files remain unchanged. Real NowSaving950 image inspected/exactsource region, write951 thenMainMenu; final1300 MainMenu image inspected. Canonical save/input/FOV/caption/texture owners and real filesystem writer are connected, with actual nonzero music/voice/SFX buffers/error0.
* Fullgate `build/native-fresh-save-gate-final.log` actualexit0:247 nativeprograms,268 Pythontests(oneexistingfilesystemskip),4referencecheckpoints,requirednative FileSelect1850/MainMenu1850/FreshSave6050(includesempty2600). Record `native-fresh-save-integration-gate.json`; STATUS generated with actual native results and N3partial. Judge fixes committedseparately5511f66 after independentreview:98.4935percent stable sourceMainMenu pixels and black-raster regression, empty01create/restart, unique reference overlays, actual native result recording. Original timing/thresholds retained.
* R4failed earlier because shared defaultoverlay contained reference-created03; untouchedOct20binary gave identicalbadimage77.14. Freshisolatedoverlay usingcurrentbinary passesOLDgolden0.0; dirtyoverlay preserved. Other failedgate attempts/provenance/UIowner/stalemtimes are recorded; none countedgreen. One earlier AudioQueueStart -66681 remains unreproduced in100-generation unit experiment; later realapp/sourceonly/fullgate runs pass, no speculativefix.
*21current sourcejudges:16strictPASS/5FPpartial/0FAIL; partialsaving_message/message_frame/center_screen_sprite/late_sprite/queue_blank_frame staypartial. Tracked summaries still need separate judge commit. Source-only closure build+populated3450journey0 already verified without originalexecutables or gameobjects; newemptychecker was verified onMain andindependentreview, not claimedreruninthatexport. Evidence `native-fresh-save-connected.json` and `native-fresh-save-production-closure.json`.
* Next actual connectedcaller CharacterSelect5114e0: Root private runtime composition `/private/tmp/sadx-native-character-runtime-stage` binds reviewedUI/preview/palette/camera and actual scheduler; NOTyetcompiled/appverified. Correct packet API names reset_input503480→existingrelease_loading_font; update_input413920→calculate_ticks. SavedFrameIncrement3b11184 is a distinct newBSSowner, notlogic_increment3b11178. Existing bootstrap.characterWORD16/menu.player_characterBYTE remain distinct.
* ReviewedCCLpacket ready `/private/tmp/sadx-native-collision-initialization-stage/frozen-collision-component-packet/packet.json`; freshreview `/private/tmp/collision-component-independent-review.json`. Actualprimaryreference/status+sameTaskWorkArena; 120624valuesexact/26568FPpartials/14mutants. Native player_startup migration with canonical primary/motion binders finishing privately under unchangedjudge; Rootneedsactual earlySonic callback+animation table/common tail/draw. No fake actor/render success.
* New source-backed CharacterSelect music3/4 maps24charactr/103trial; private imports deliveractualsamples. DTO/onecombined3302pack+176voices packet `/private/tmp/sadx-character-selection-assets-stage/packet.json`; all162oldclips retained+14sourceclips. Source cue2047 is absent BOTHlanguages, explicitmissingasset, no substitute/suppression. Rootmustintegrate surgicalhooks/reader andpreserve onepack. Referenceclean2467command replay running detachedapp82604/driver82600; exact throughChaos6020 sofar, no newnativeclaim; liveobserver68216/driver68215 preserved30238MysticRuins. No pushes; complete native story goal remains active.

### 2026-10-21 — Native CharacterSelect caller, resource and first actor boundary (Root)

* Fresh-save creation/restart/empty01 is committed in f8c2875; required6050-frame native check passed in its recorded247-native/268-Python gate. New separate judge5311f40 registers CharacterSelect fidelity/resources. The actual Main native app now follows startup→fresh03→MainMenu→Adventure through CharacterSelect init/draw, palette2, seven real flags7 tasks and camera factory. Task/motion bindings share canonical owners and preserve Sonic aim16384. Real Question-first exec updates yaw16384→17112 and executes shared matrix/specular29/menu2 prefix; it then stops loudly at `native_basic_object_renderer`, frame1382. The first UI frame is source-transparent and still shows the outgoing logo: no full CharacterSelect/actors/image claim.
* One source-backed3302-record combined graph, label/completion metadata, dialog7–10,5music requests and176voice clips import through the real importer. All162 previous voice clips retained; missing source cue2047 remains an explicit request boundary. Actual composed CharacterSelect music delivery remains unverified while the frame stops; isolated24/103 sample-delivery units pass. Existing controller/backend and local audio gains(.05/.30/1.5) unchanged. No gameplay63FPS claim.
* Native source build/import0;8new registered units0. Main original-source judges have exact stored values: availability/preview/question/completion strictpass; menu733FP, camera25152FP, lighting11FP, labels2014FP, FileSelect340FP, PInitialize924FP remain partial. Camera canonical target exposed its new fingerprint-path portability defect after executing cases; the unchanged judge reran using a private copy of Mainprobe and identical Main source files. Separate judge correction pending. Fresh independent composition review caught/fixed Work.selected_character int32 vs globalbyte substitution using three original-ABI discriminator cases; complete source caller map retained. No thresholds, expected values or original verifiers changed in the port.
* First coherent Main fullgate actualexit2: legacy `player-auxiliary-effect-test` needs the now-reference-only Startup facade linkage. Python source-backed tests also expose private-only ROOT setup/Capstone interpreter prerequisites; fix separately, never skip. Evidence `reference/production-integration-20261021/native-character-select-caller-frontier.json`, raw logs `build/native-character-*`. Fullgate/STATUS refresh still pending these judge registrations. Root owns Main source/build; workers write private packets. No assets/binaries committed; native symbols contain no guest/Unicorn/legacy Startup facade dependency.

| Next actual caller | Canonical owner | Missing behavior/resource | Existing packet | Owner | Integration check |
|---|---|---|---|---|---|
| Question5125a0→dsDrawObject408530→DrawObject4034b0 | Display fog/queue/matrix/texture; one mutable object graph | actual object traversal/model draw77ef70 | Question approved; object_draw private strict2853 initialpass | Root integration/native_menu_judge recovery | real source prefix, shared graph identity, actual Metal model/image |
| Gamma483430 mode0/ID2, next scheduler actor | actual primary/motion; bootstrap clock/cutscene owners | PInitialize6, animation table, CCL/texture/event/weld/common-tail | PInitializev3/CCL/Sonic-entryv2 shared dependencies; Gamma recovering | sonic_entry_review worker; Root caller | normal reached initializer, mandatory common-tail, real callbacks |
| Sonic49a9b0 and remaining four actual actor callbacks | Task and one Registry/current state | complete constructors/update/display/animation | current-statev2 and Sonic-entry/catalogv2 independently approved; Sonic constructor tail/CCL approved | Root integration | no pose-only/seeded-tail substitute; input/cancel/reentry and natural Sonic story entry |

* Reference only: clean full2467-command EC/train replay completed77.45min,410/410capture state+images and finalsave byte-exact. `reference/story-route-20261020/clean-replay-20261021/final-verdict.json`. Original observer68216/driver68215 preserved; reference_ec_continuation owns new normal-input acquisition toward Egg Hornet. This does not satisfy native gameplay. Complete native Sonic story goal remains ACTIVE; continue after this integration/gate/commit.

### 2026-10-21 — User pause, process shutdown and requested push (Root)

* User explicitly paused the complete native-story goal, requested all running project processes stopped, and authorized pushing current work to main. All workers stopped. Gate/native test applications and original observer68216/driver68215 terminated; the earlier preserve-live instruction is superseded. No further implementation, recovery, builds or gameplay runs are authorized until resume. Logs, replay files and private recovery work remain on disk.
* Committed native frontier0600979: normal startup/fresh save/MainMenu→CharacterSelect preparation, real seven-task/camera creation and Question yaw/matrix/specular/texture prefix; explicit `native_basic_object_renderer` stop at1382. First UI frame remains transparent/outgoing logo. Full CharacterSelect and connected gameplay remain incomplete. Source build/import0;8new native units0;312Python tests0 with one pre-existing filesystem skip. FP partials remain partial.
* Separate judge corrections now qualify reference-only auxiliary-effect test linkage, source-root defaults, Python Capstone dependency via existing reference venv and canonical camera report fingerprints. Full camera26808cases still0value/25152FPpartial. The legacy menu-voice test retains all81requests/162requiredclips while14newCharacterSelect clips coexist; removing one old clip still aborts the exact162 assertion (mutant detected). New selection unit separately verifies176clips. Earlier fullgates failed2 at linkage then legacy clip-total fixture; both actual failures retained.
* Final `make -j10 gate` was STOPPED at user request: actual make exit−15/SIGTERM (`build/native-character-integration-gate-final-exit.json`). It reached native journeys after all native programs/Python discovery and4/4reference journeys; FileSelect1850 finished, MainMenu was interrupted. This is NOT a green full gate; do not regenerate STATUS as successful. Evidence `reference/production-integration-20261021/native-character-check-prerequisites-judge.json`.
* Reviewed private recovery remains LOCAL, not integrated/pushed: `/private/tmp/sadx-object-draw-stage/packet.json` (2918/5918strict,15mutants; fresh independent review approved); `/private/tmp/sadx-native-sonic-current-stage/frozen-current-state-packet-v2/packet.json`; `/private/tmp/sadx-native-sonic-entry-stage/frozen-sonic-entry-packet-v2/packet.json` (fresh stable-archive review approved); `/private/tmp/sadx-native-gamma-entry-stage/frozen-gamma-entry-packet/packet.json` (prefix only; independent review pending). Root shared mutable object owner/readiness bridge under `/private/tmp/sadx-native-character-runtime-stage` has independent approval and2918 unchanged object-source cases0.
* UNFROZEN private work preserved as-is: `/private/tmp/sadx-native-texture-selection-stage` (plain native selection/shared source algorithm; complete683legacy and3683/3683native strict,16mutants; no finalpacket); Gamma Stand extension in `/private/tmp/sadx-native-gamma-entry-stage` (true Stand3303, samegraph3324records, old3302prefix/22bindings exact; no finalresourcepacket). No game assets/binaries are committed.
* Resume at the actual missing caller: complete real403070 queued/current texture selection and cache binding before object drawing. Existing LogoRenderer only selects the current list; leaving queued-list NULL would falsely skip models via403250. Bind approved shared mutable graph/object traversal, then actual model77ef70; next scheduled actor is Gamma mode0/ID2. Retain all mandatory actor callbacks/common-tail/animation/collision/audio, normal Sonic story entry and the full acceptance route. Re-run the required full gate after resume; no completed-goal claim.

### 2026-10-26 — DiveMac orientation and controller dependency repair (track J, no green gate)

* Docs were already committed in da25501. Login Python is /opt/homebrew/bin/python3; REA Ghidra doctor and both app builds pass.
* All three native journeys pass: FileSelect 1850 frames, MainMenu 1850 frames, fresh-save 6050 frames. CharacterSelect still stops at native_basic_object_renderer.
* First full HEAD gate exits 2: windowed app-smoke crashes in libusb's unconditional call to the absent weak pipe2 import.
* The dependency builder now selects upstream pipe/fcntl for the macOS 14 deployment target. Identical SDL gamepad-init subprocess: copied library SIGSEGV, rebuilt library exit 0.
* The repaired windowed smoke reaches the AppKit run loop but receives no display frames; macOS screen capture also reports no display. That run was terminated, not counted as passing.
* Evidence: reference/production-integration-20261026/controller-pipe-portability.json. Physical controller delivery and the complete gate remain unverified.
* Next: run the full gate with a visible DiveMac GUI session, then land the prepared GOAL section 6 item 2 tooling packets.

### 2026-10-26 — Green gate on DiveMac at the CharacterSelect frontier (track J/V)

* Outcome: make -j10 gate exits 0 at 9e10e47 after the libusb deployment-target repair. The visible GUI session also passes the 120-frame windowed smoke check.
* Reference R1 through R4 pass. Required native FileSelect 1850, MainMenu 1850 and fresh-save 6050 frame journeys all pass.
* The existing independent startup comparison also passes all 16 exact state/image checkpoints. Python discovery passes 312 tests with one pre-existing filesystem skip.
* Gate evidence: reference/production-integration-20261026/divemac-head-gate.json. STATUS was regenerated from that real result.
* Native make play-native still reaches menus and creates/loads saves; CharacterSelect remains stopped at native_basic_object_renderer, frame 1382. No gameplay or 63 FPS claim.
* Next: integrate GOAL section 6 item 2 packets, acquire the two missing reference return-transition captures, then recover the next reached CharacterSelect dependency. No push.

### 2026-10-26 — N3b dependency map before packet recovery (track J/S, tooling gate pending)

The native frontier remains Question drawing at frame 1382. Integrate in reached-call order; each packet must pass its unchanged judge before use.

| Actual caller/callback | Canonical owner | Missing behavior or resource | Packet | Owner | Integration check |
|---|---|---|---|---|---|
| Question selects menu textures; njSetTexture 403070 | Display current and queued lists, texture pool/cache and palette state | Queued/current selection and real surface cache binding | packet/native-texture-selection, to recover from rollout | Root | Queued list is live before 403250; unchanged selection oracle and cancel/reentry |
| dsDrawObject 408530 to DrawObject 4034b0 | Session single CharacterPack; shared mutable object view still to bind | Guarded traversal preserving object identity | packet/object-draw, to recover | Root | Real Question traversal reaches model draw; unchanged oracle and graph alias checks |
| Model draw 77ef70 | Display matrix, shared graphics Draw and Frame | Original basic-model conversion and actual Metal submission | Next reached renderer packet | Root | Source-backed Question pixels and draw-suppression mutant |
| Gamma mode 0 / ID 2, Sonic 49a9b0 and other preview callbacks | Actual Task primary/motion, player registry and imported actor tables | Full initialization, CCL, animation, common tail, update and display | Durable actor packets after their dependencies | Root | All real actors draw and survive cursor/cancel/reentry; no static substitutes |
| Character menu confirmation to Sonic story | Bootstrap/menu/save owners and actual sound resources | Normal confirmation and first story callee; cue 2047 stays explicit missing asset | Reached story packet | Root | Natural Sonic entry, source images and delivered music/voice |

### 2026-10-26 — GOAL section 6 item 2 tooling integrated (track V, gate rc 0)

* Outcome: the full strengthened gate passes at b833ed4. Native menus/save paths remain visible with make play-native; CharacterSelect still stops at native_basic_object_renderer, frame 1382.
* Native closure force-loads every archive member, checks nm symbols and keeps the unchanged 125-source product set in native-sources.txt. The hidden ground-acceleration guest-link mutant fails both checks.
* Readability now ratchets large decimals, emulator arithmetic and address comments: baseline totals 233, 3322 and 1059. Naming constexpr literals changes the metric, not the game source.
* All 50 native captures and reference R2/R3 require qualified content; genuine clear/rich-raster mutants fail. Both return transitions retain source frame 1450 and the unchanged ceiling; exact pixels occur one source frame later.
* Startup parity is required in the gate. A strict 520-frame normal-input trace stays failing on unrecovered native player/camera owners and the movie-skip mode difference, native 452 versus reference 453. No mismatch is aligned away.
* Gate rc 0: 255 native programs, 364 Python tests with one existing filesystem skip, R1 through R4, three native journeys and 16 exact startup comparisons. Evidence: reference/production-integration-20261026/tooling-integration-gate.json; STATUS generated.
* Stale census guidance and generated status were corrected; pre-October 19 journal text is preserved verbatim in docs/history. No game oracles were changed or rerun for this tooling step; existing partials/failures remain.
* Next: one bounded extraction attempt for texture selection, then fresh recovery if incomplete, under unchanged oracle/mutants/holdout. Packet branches/worktrees are durable; no push.

### 2026-10-26 — Bounded packet salvage and fresh recovery started (track S, no new gate)

* The reusable static extractor is committed in ca14482; 18 focused tests pass and are registered in Python gate discovery. Transcript commands are never executed.
* One texture-selection attempt recovers nine authored files but reports 196 unsupported operations; one object-draw attempt recovers zero files and reports 280 unsupported operations. Both are incomplete and untrusted.
* Raw manifests/snippets stay in build/salvage. Small evidence and delegated task identities are in reference/production-integration-20261026/packet-recovery-started.json.
* Fresh texture recovery runs on packet/texture-selection-core; fresh object wrapper/traversal recovery runs on packet/object-traversal-core. Both own private worktrees and builds; Root owns composition and the next full gate.
* Object source dossiers survive under reference/inventory-raw/native-object-render-recovery-20261021. Basic-model dossiers also survive under native-basic-model-render-recovery-20261021; context_bundle needs an explicit nested cache directory to find those.
* Root has not trusted or integrated recovered behavior. Require exact source/oracle evidence, mutants, holdout and fresh STYLE review before each packet is consumed. No Ghidra during a gate.
* Native make play-native still reaches menus/save paths and stops at native_basic_object_renderer, frame 1382. Next dependency is real queued/current texture selection and cache binding, then shared graph traversal and model drawing.
* Last full gate remains tooling-integration at b833ed4, rc 0; no gameplay, CharacterSelect completion or 63 FPS claim. Continue automatically when the active recovery tasks return; no push.

### 2026-10-26 — Native shared object/texture composition at the real model boundary (track J/S, gate rc 0)

* Approved texture core 3878f33, typed gate and object traversal now share Display's actual owners and Session's one mutable ObjectState. Root code is frozen in 940dca3; the normal native route reaches native_basic_model_renderer at frame 1382.
* Source-only archive builds without original EXE/DLLs or prebuilt game objects and reaches the same model boundary; its image is byte-exact to Main. Both still show the outgoing title fade, not CharacterSelect actors.
* Root preserves strict 683 legacy, 3669 development/holdout texture cases, 4256/4096 gate cases and 2918/5918 object cases. Four binding mutants fail assertions; object mutants remain 18 oracle plus one alias-unit detection. Downstream sprite/queue/color/FOV partials remain partial.
* A context review found the SpriteImmediate caller is 78d140, not 77f440. The first combined gate was stopped with actual rc 2 before recording green. Correction cb86c98 preserves requested memory and returns cache format; 3408 development and 5408 holdout cases match, and five mutants fail.
* Fresh correction review ced9052 approved; corrected source-only archive also builds and reaches the identical frame/image. Current source/report commit is 1f13f69. No old oracle, expected input or tolerance changed.
* Combined gate at 1f13f69 exited 2: two archival replay checks used mutable build paths. The no-display windowed smoke also waited and was terminated; no green gate was recorded. The separate corpus judge correction preserves all expected hashes.
* Combined gate at 04c40ae now exits 0: 262 native programs, 386 Python tests with one existing filesystem skip, R1 through R4, three native journeys and 16 startup comparisons. Record: reference/production-integration-20261026/object-texture-integration-gate.json; STATUS generated.
* Ten fresh strip, buffer and CorrectMaterial dossiers are retained on packet/normal-uv-strip at 5eb554c; Ghidra closed before the gate. The next reached call is still njDrawModel 77ef70. Independent strip and model packets plus Root material correction are under verification, not runtime claims.
* Default/alternate object rotation, late queue and real model rendering remain named loud seams. Native gameplay, full N3b, controller requalification and 63 FPS remain unverified. No push.

### 2026-10-26 — Reached model closure recovered; native binding in progress (track J/S)

* Main retains the recorded gate rc 0 at 04c40ae: 262 native programs, 386 Python tests, R1 through R4, three native menu/save journeys and 16 startup comparisons. The native app still stops at native_basic_model_renderer, frame 1382.
* Original strip and PolyBuff helpers are frozen at 1960fed; CorrectMaterial at 1a5a0f3. Fresh review 95913d8 approves both bounded components. Their FP partials remain partial; no GPU or N3b approval.
* Material correction has 4227 development and 6227 holdout cases with zero value mismatches, 3231 and 4567 flag differences, one ordering unit and five caught value mutants. Strip has 1014 and 2662 cases, 154 and 198 flag differences, 12 checks and seven caught mutants.
* Model wrapper/material/environment recovery is frozen at 9220d1b. It preserves 2427 and 3427 cases with zero value/order mismatches and 172 and 274 FP partials; seven mutants are caught. Its original oracle, inputs and API/state receipts stay unchanged after fixing the live float-matrix borrowing interface. Fresh review is active.
* CorrectMaterial Power belongs to the existing active lighting palette: LSPalette plus SP_pow offset 48. The reached Question material requires environment mapping. The source evidence and canonical mapping are in basic-model-material-ownership.json.
* Root integration lives on packet/basic-model-binding in /Users/burbujamc/Developer/sadx-wt-basic-model-binding, at 5ccc04f. Reviewed strip/material sources are byte-identical; 878 judge inputs have zero drift. Native runtime attachment and product source registration remain pending.
* Full InitPolyBuffers and its shared legacy owners are being migrated on packet/polybuff-initialization under unchanged legacy judges. Do not duplicate the nine buffer heads, scratch allocation or clear block, hardcode a capacity, or fake initialization of only the reached buffer.
* Next: close model review and initialization ownership, bind the real shared device and generated texture coordinates, run normal-input Question output, then continue the actual actor controllers. All Ghidra providers are closed. No push.

### 2026-10-26 — Native model pipeline reaches Gamma (track J/S, combined gate pending)

* packet/basic-model-binding now reaches native_gamma_player_controller at frame 1382 under the same normal fresh-save input. Native app build and forced closure exit 0; diagnostic journey exits 1. Main's last recorded full gate remains 04c40ae, rc 0.
* Source buffers, shared geometry, material correction, environment coordinates, transforms, strip conversion and native frame submission are bound. Runtime stage faf1d87 received bounded review 635df95; no actor pixels or N3b approval.
* Root found and corrected an untextured-material adapter mistake in 004d03f: 78bba0 must unbind the cache, not call 77fc50 texture cleanup. Regression checks preserve loaded references/surfaces and the cache minus-one guard.
* Five native binding mutants compile and fail assertions: suppressed draw, missing stream retain, wrong Power, swapped vectors and swapped view/reset owners. Earlier component FP partials remain partial.
* Full canonical initializer bdf110a received fresh review afc50a7 and is integrated at 7bacea7. Actual Display startup a316c9a initializes all nine buffers from the live library count state, allocates scratch, retains only workspace identity and discards buffers at frame begin.
* Rendering composition now uses actual startup, with the test-only buffer owner removed. Device, geometry and composed model units pass. Native/reference coexistence runs actual native constructor and nine reference calls in one process, exit 0. No direct-workspace byte consumer is recovered.
* Latest binding branch commit is 46f561f in /Users/burbujamc/Developer/sadx-wt-basic-model-binding. Fresh startup-binding review, explicit fixture-only build rules, judge registration, source-only product repeat and combined gate remain pending.
* Gamma recovery runs on packet/gamma-selection-core; shared current-owner migration runs on packet/player-current-owners under unchanged Sonic judges. Gamma borrows the shared slots and never adds shadows or super-byte writes.
* Fresh full E102 483430, InitGammaAnimData 7ca300 and InitGammaWeldInfo 7cb560 dossiers are retained at packet/normal-uv-strip commit 0eed0d8. Ghidra exited 0 and closed before gate preparation; no 7cdd60 guess was queried.
* Continue through actual actor callbacks; all-preview initialization/update/drawing, cancel/reentry, story entry, gameplay and 63 FPS remain unverified. No push.

### 2026-10-26 — Model source-only repeat and gate preparation (track J/V)

* Binding branch packet/basic-model-binding is clean at 1b7b94d. Its maintained-source export 88eb56e builds without game EXE/DLLs or prebuilt game objects, reusing only pinned source-built SDL/libusb.
* The exported native app follows the same normal fresh-save input to native_gamma_player_controller at frame 1382, actual exit 1. Capture 1382 is byte-identical to the binding frontier. No all-actor pixels or N3b claim.
* Explicit compatibility recipes apply guest/polybuff_owners.hpp only to frozen fixture TUs; no global or inherited CXXFLAGS changed. Seven legacy/new units pass with original manifest arguments. An initial ninja-view invocation omitted its required trig argument and failed usage; corrected invocation passes.
* Root reran unchanged system init 600, device setup 2000 and frame calls 3969 cases with zero mismatches; the default initializer target passes 152 cases. Producer dev/holdout and inherited FP partial limits remain separate.
* Symbol generation now uses pinned source headers/labels with read-only local Git metadata. Provenance, readability, layers and owner checks pass; the updated judge manifest protects 893 inputs with zero drift. No original expectation or tolerance changed.
* Source-only and setup receipts are committed under reference/production-integration-20261026 in the binding branch. Final startup and shared-current review is active; it must finish before the combined full gate and Main code integration.
* Shared current-owner migration 28b3974 on packet/player-current-owners preserves Player 28800/2840 FP partial and Actions 30192/2172 FP partial, three old units, 257 worlds and seven mutants. Root review/native consumption remain pending.
* Gamma core recovery uses actual 483430 source. Its body has no direct event 580 call; 49fb10 and 4c5420 remain neutral setup seams. Full source corrected earlier guesses; no guessed event or TimeZero implementation is accepted.
* Main's last recorded full gate remains 04c40ae, rc 0. Continue final review/gate, then the actual Gamma callback and its reached dependencies; the complete Sonic story route remains active. No push.

### 2026-10-26 — Model landed; windowed gate stopped without display frames (track J/V)

* Main 6b617c8 contains the reviewed model/material/geometry/device and canonical nine-buffer startup closure. Normal input and the source-only export both reach native_gamma_player_controller at frame 1382, actual diagnostic exit 1. N3b remains open.
* Full make -j10 gate exited 2. Compilation, unit and Python sections completed, but required windowed smoke PID 41327 emitted zero frames while macOS reported Display Asleep: Yes. The process sample showed the AppKit event loop idle.
* A bounded caffeinate assertion woke the display; normal NSRunningApplication activation returned false and the smoke stayed idle. Root killed only that exact stalled smoke, then observed make return 2. No green gate was recorded. Evidence: model-startup-integration-gate-failed-display.json.
* User GUI readiness is pending. Require a fresh real windowed smoke before rerunning the full gate; do not infer readiness from screencapture alone or alter gate expectations. Last recorded successful full gate remains 04c40ae, rc 0.
* Gamma's build pause is revoked after the actual gate terminal. Its final source/oracle/unit/mutant/holdout work may resume privately; no Ghidra or app launch without Root coordination.
* Root prepared the approved shared current owners in packet/gamma-runtime-binding, /Users/burbujamc/Developer/sadx-wt-gamma-runtime-binding: db0edf6, b00f413, 281b6de. Core-only actor-current unit exits 0. No Main code or gate source was changed during the run.
* Continue GUI smoke/full gate and the reached Gamma callback. All actor drawing, cancellation/reentry, Sonic confirmation, gameplay and 63 FPS remain unverified. No push.

### 2026-10-26 — Native model/startup full gate green (track J/V)

* Full make -j10 gate retry exits 0: 270 native programs, 386 Python tests with one existing filesystem skip, R1 through R4, three native menu/save journeys and 16 startup comparisons. Record: reference/production-integration-20261026/native-model-startup-gate.json; STATUS generated.
* Fresh windowed smoke drew 120 frames after the user restored the visible GUI. Prior display-stalled attempt remains recorded as rc 2; no checker or expectation was changed. All providers were closed during both gates.
* Native source-only export and normal fresh-save input reach native_gamma_player_controller at frame 1382, diagnostic exit 1. The inspected capture is still outgoing title fade; no all-actor or N3b qualification. Earlier FP partials remain partial.
* Gamma mode 0/common-tail core 3477fda received fresh bounded review 0acbb8f. Root imported it only into packet/gamma-runtime-binding at 9f5877a; actual lower calls and product binding remain pending. Shared current owners 28b3974 are in that same worktree.
* Gamma asset worker resumed after the actual terminal. Strict import produces 3328 records, preserving 3302 existing encoded records and 22 bindings. Original recipes/weld and inverse checks pass; final review and product consumption remain pending.
* Matrix numbers 54, 30, 39, 2 and 37 are animated Ninja preorder cache indices for GetMMMatrix, not E102_OBJECTS export rows. Root independently confirmed the 77-node child/sibling traversal and exact pointer identities. Matrix contents require the actual animation cache; no static-pose fallback is accepted.
* Next: finish Gamma assets/review and current-owner fixture validation, bind the actual controller with existing native PInitialize, then recover its first real missing lower call. Continue all preview actors, cancel/reentry, audio, Sonic story and the full route. No push.

### 2026-10-26 — Native Gamma initialization reaches actual CCL boundary (track J)

* Private packet/gamma-native-binding connects the actual TitleScene callback, shared current State and eight-slot Registry to real PInitialize, imported parameters and all 91 animation recipes with real Stand 0. Source algorithms and frozen judges stay unchanged.
* Normal fresh-save input exits 1 at frame 1382 at native_gamma_CCL_Init. Inspected screenshot remains the outgoing title screen. No actor-pixel, N3b, GPU or story claim.
* Actual binding unit, frozen Gamma core unit and frozen startup unit exit 0. Consumer checks distinguish actual bootstrap timer bytes and cutscene task nullness from unrelated state. Force-load native closure exits 0 without guest or PE dependencies.
* Unchanged startup oracle rerun has 2,004 cases, zero value/boundary/resource/control mismatches and 924 FP flag partials; existing script exits 0, verdict remains partial. Inherited Gamma 1,422/1,080 cases remain partial with 498/690 FP differences and rc 2.
* Proof, source hashes, exact app command and limits are in reference/production-integration-20261026/gamma-native-binding. Compiler/test registration and receipt refresh are separate from implementation. Root owns fresh review and full gate; no provider, full gate or push run here.
