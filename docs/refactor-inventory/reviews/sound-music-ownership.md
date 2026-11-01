# Sound and music ownership review

Review base: `e5cf19d7daa984ff9df566a6dcd7d6c9ed39a132`. Authored in isolated branch
`packet/sound-music-ownership-review-e5cf19d`. No source, Main, global inventory,
generator, shard, judge manifest, HANDOFF or STATUS changes. No builds, tests,
oracles, mutations, providers, network, storage operations, gate or delegates run.

All 20 requested files were read completely: 1,602 lines. The fragment records
independent per-file reasons and explicit complete membership for all ten modules.
Only the selected numeric initialization module is conforming; paired file statuses
do not imply closure. Consumer files below are evidence, not certified by this review.

| Complete file scope | Disposition | Reason |
| --- | --- | --- |
| `audio_stream.hpp`, `audio_stream.cpp` | needs_migration | Game-tier fixture availability, source addresses and register-shaped results; preserve initialization ordering. |
| `sound_manager.hpp`, `sound_manager.cpp` | needs_migration | Legacy recovered server policy duplicates the native core; preserve survey/failure boundaries while sharing behavior. |
| `sound_initial_state.hpp`, `sound_initial_state.cpp` | already_conforming | Complete numeric template and constructor; matching byte/source evidence and three caught numeric mutants. |
| `audio_playback.hpp`, `audio_playback.cpp` | open_review | Semantic resource owners; receipt lacks maintained-source binding and full mutation/holdout evidence. |
| `music.hpp`, `music.cpp` | open_review | Semantic scheduling/reader policy; two bounded source prefixes and missing raw/mutation/holdout evidence. |
| `voice_playback.hpp`, `voice_playback.cpp` | open_review | Semantic canonical-reader wrapper; missing raw/mutation/holdout receipts. |
| `sound_bank.hpp`, `sound_bank.cpp` | open_review | Effective loader policy resolved; incomplete current implementation/normalizer qualification. |
| `sound_effects.hpp`, `sound_effects.cpp` | open_review | Semantic native core; legacy duplication and missing per-case FP/mutation/holdout receipts. |
| `level_sounds.hpp`, `level_sounds.cpp` | open_review | Semantic borrowed policy; historical cleanup dependency hashes differ from current source. |
| `menu_voice.hpp`, `menu_voice.cpp` | open_review | Delayed voice task identity resolved; missing raw/mutation/holdout evidence. |

Result: **2 conforming, 4 migration, 14 open files**; **1 conforming, 2 migration,
7 open modules**. No file is obsolete or pure reference infrastructure.

The next canonical owner should be a bounded **music scheduling state** holding the
existing `TrackState`, `VoiceTiming` and manager `TaskHandle`, owned by Session in a
dependency-light game/sound header. These values jointly implement request publication,
duration fallback and manager lifetime. Moving their declaration alone is insufficient.
Keep `PcmState`, `MediaState`, numeric `SoundInitialState`, bank slots/channel bindings
and scene-owned request tasks separate. Configuration, language, catalogs and readiness
remain borrows; no universal Services/Audio framework is needed.

The exact consumer closure is:

1. Carry `music` play/query/init/update, `menu_voice` publication, cleanup song/voice
   reset and `title_menu` manager close/delete together. Manager deletion resets five
   track fields but leaves `real_previous_song` untouched; do not merge reset bodies.
2. Rebind Session `tracks_`, `voice_timing_`, `sound_manager_` and TitleScene manager
   load/update/delete/stop and request callbacks. `sound_.volume_task` in TitleScene
   is the delayed **voice** request; it is distinct from the manager and music request.
   Task auxiliary native work owns each request. Preserve publication-before-allocation,
   previous elapsed > 0, texture readiness and canonical clear after task destruction.
3. Preserve scheduler identity during destructors which clear the canonical task handle.
   `docs/HANDOFF.md` records the actual Start-transition crash from borrowing that
   handle by reference; `tests/unit/task_callback_lifetime.cpp` is an affected regression.
4. App `configure_music`/`configure_voice` and cold-start initialization continue to
   borrow the existing media players. Keep live attenuation, source-owned music restart,
   platform looping disabled and presentation-clock completion distinct from token loss.
5. **Preserve `pcm_.initialized` as the exact reference to
   `Session::sound_.stream_initialized`.** `sound_` is declared first and outlives PCM.
   Title readiness, bank selection, server resources and cleanup share that word.
   Do not add a readiness cache, reset it per frame or invalidate captured bank/SFX views.

Reference music/reset closure belongs in that first packet too. `ResetLeafState`
currently owns the six optional words corresponding to native `TrackState`:

| Native track field | Reference source word |
| --- | --- |
| current_song | `0x912698` |
| previous_song | `0x91269c` |
| real_previous_song | `0x9126a8` |
| current_voice | `0x912694` |
| song_queue_state | `0x3b29cf8` |
| voice_queue_state | `0x3b29cf4` |

`game_reset_leaves.cpp` duplicates the two cleanup reset bodies at `4257c0` and
`4257e0`. Carry NativeGame constructor/import/reset composition and the composed
reset probe through shared semantics with reference availability outside the game
owner; do not seed unknown PE facts with native defaults. Adjacent reference
WMPClose/WMPInit player/status/config logic and cold-only StopPCM remain separate
recovered migration debt. The composed reset judge preserves its initialized-audio
`41010d` boundary and byte-page/call comparisons; it cannot be silently replaced
by the broader native PCM judge.

The original music judge maps duration/start to `3b29cec` / `3b29cf0`, manager task
to `3b29d00`, and the two scene request slots to `3c5e8c8` / `3c5e8cc`. A maintained
reference owner for those timing/task locations was not found by source search;
keep that fact explicit. `3b0ef38` is voice language, owned by WinMain reference
state and borrowed in native Session; it must not be mistaken for the manager.

A later reference-server packet must share recovered `sound_manager` behavior with
`sound_effects`, rather than merely relocate it to guest. Reference timers/pause/frame
belong to `AudioStartupState`; flags/entities to `ResetLeafState`; remaining record
fields/listener to `SoundRecordFields`. Their optionals describe missing fixture facts.
The legacy `frequency` field is pan angle and `pan` field is pitch. Keep lazy availability,
partial stores, register marshalling and missing-return survey behavior in the adapter.
The native server is broader on active records, but its judge does not replace these
contracts. Separate native/reference state instances are not duplicate live ownership;
the duplicated recovered server algorithm is the debt.

Actual resource absence includes null bank/player/task, missing bank file, absent sample
data and filtered underscore-prefixed bank names. Unknown outer optionals in stream
initialization and legacy sound fields are not game absence. Out-of-catalog duration,
unsupported encoding and missing sample-name indexing stay explicit boundaries.
Platform 3D/distance/listener/queued-SFX-voice/variable-frequency implementations still
throw at their unrecovered operations; this review does not complete those behaviors.

Evidence and prerequisite limits:

- Historical data judge: one case, 329 numeric words / 1,097 source bytes. Both numeric
  source files and probe/judge hashes match; all three numeric mutants were killed.
  Four bridge mutations and constructor/CLI comparisons are historical wider evidence;
  current bridge source differs, so this closes only numeric initialization.
- Historical music: 1,301 observations comprise 1,299 full executions and two bounded
  prefixes where callback mutation exposes duration index -1. Do not call all 1,301 full passes.
- Legacy server: 956 comparisons and **544 skipped active-record inputs** out of 1,500.
  Freeze exact skipped IDs and native outputs before any rewrite.
- Native queue/server reports: 2,580 / 1,956 observations. Parameter values match in
  2,440 cases, with **553 FP-flag differences**: partial remains partial. No exact FP
  identity/full-output ledger is present here. HANDOFF older server 2,084 / 2,468 and
  parameter holdout 3,488 / 920 partial counts do not prove current cohort equivalence.
- Voice wrapper: 1,000; menu voice: 4,888; level selection: 2,296 observations. Their
  own source maps match. Level selection cleanup dependency hashes differ. Playback
  (1,000 PCM / 44 media / 2,010 config) and bank (705 selection / 700 release) summaries
  omit complete maintained-source bindings. All counts are old receipts, not reruns.
- Twelve scoped oracle-script hashes match the protected manifest read-only. This is
  not a full manifest validator, compiled probe identity check or new judge pass.
- Original raw dossiers referenced by numeric evidence are absent here. Complete
  compiled mutation/holdout receipts for the seven open modules were not found in
  tracked local reference files. Retrieve/qualify them before source edits; prose
  counts and the 162-clip import qualification do not replace original wrapper judges.
- Pinned profile keeps the unconditional loader `LoadSoundList_r` replacement active
  while BASS SFX replacements are inactive. `MediaFns.cpp` selection and `dllmain.cpp`
  hook were inspected through the pinned SDK gitlink. Do not port stock SetBankDir.

Root prerequisites: fresh affected baseline and exact inherited boundary/FP identities;
separate evidenced judge binding changes where needed; unchanged original cohorts and
mutant intent; callback/alias/lifetime checks; native source build/closure and actual
menu/save journeys including audio events. Windows/device/race/performance and codec
equivalence remain unqualified. Root gate remains outside this review.

The JSON records full SHA-256s for every reviewed file, actual consumer evidence,
historical reports/source ranges, scoped judge identities and external gitlink boundary.
Selected supporting fingerprints:

| Evidence | SHA-256 |
| --- | --- |
| `src/runtime/native_session.hpp` | `3326f3e882138f59b3791ecfa8bec3cf8b07e7c6daf087fefbbe37b322ebe23c` |
| `src/runtime/native_session.cpp` | `39435635199aa8f5f763af6ead381211d00cb1f93da6ab4a2572e182a118fd75` |
| `src/runtime/native_title_tasks.cpp` | `e41744c5417fbc793679a62b72eeeed3d17ca9c6aed037889fd0496b03dc8f44` |
| `src/game/game_cleanup.hpp` | `5e75754bfcdb7823872c06189ad556c440f95788e11a6fa2382b1db2a715cff7` |
| `src/game/game_cleanup.cpp` | `024c23e264893b083234cd4acd187a9f08496a9aae650f21a446740cb8f46b9b` |
| `reference/oracle-reports/sound_initial_state.json` | `173e22ab74a3c6fe0df5eb09238984813dd92fee9c6ae83ba269d927e764dac2` |
| `reference/production-integration-20261016/sound-initial-157-numeric-mutants.json` | `e6a0be4eeaa70cb513d4268a56b13ed0d9162dba6c85b1ddb180755a1eab482e` |
| `reference/oracle-reports/music.json` | `501e260ca894829d5c481fed5db7909787dd3dc9de6f387de43807843e80c211` |
| `reference/oracle-reports/sound_parameters.json` | `b089ab9904d2b2788530d333ce9cd644a853f165435cf47fe2e8e633563a335e` |
