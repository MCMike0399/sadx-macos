# Game reset: source context and ownership closure

Research packet at Main `fdb1595`, 2026-11-01. Scope: `game_reset.{hpp,cpp}` and
its real consumers; **not implementation, subsystem approval, or a fresh verdict**.
No build, oracle, mutant, holdout, provider, network or storage-management operation
was run. Main's concurrent render gate and every existing writer were left alone.
Only this document and its [mapping](context-game-reset.mapping.json) are changed.

Recommendation: do not schedule a standalone “rename GameResetState” migration.
There are already **two recovered Reset algorithms**: the legacy address dispatcher
and readable `cleanup::reset`, used by native Session. Share the latter's behavior,
but first resolve the shared frame/editor owners and reference partial-failure
contract. A bounded orchestration consolidation is possible; it cannot certify the
whole reset/level/task/audio owner closure.

## Evidence and confidence

Instructions: AGENTS; GOAL §0's active ADR-0009; STYLE §§2–7, 10, 12;
ARCHITECTURE's two-tier destination; PROMPT-complete-refactor §§1–4;
refactor-inventory README, PACKET, and review.json's game_reset/Sonic findings.
Inventory remains an open semantic review, not proof of closure.

Main means `/Users/burbujamc/Developer/sadx-macos`. Symbols were read from Main's
existing `build/symbols/symbols.json`, loader commit
`eb4cdcca9637d611a98b910fcfed6ce18580451c`. No symbol regeneration. Mapping records
SDK declaration locations, cached dossier evidence IDs/body bounds, and file hashes.
Cached `reference/inventory-raw/` is local ignored evidence on Main; it is not
copied into this packet. All source paths below are relative to the repository.

Primary evidence keys:

- **R**: `src/game/game_reset.cpp:5–34`, header:13–33; unchanged reset oracle/probe.
- **C**: `src/game/game_cleanup.cpp:225–255`, header `State`, `Services`, `reset`;
  native Session `reset_game` at `src/runtime/native_session.cpp:130–176`.
- **L**: `src/game/game_reset_leaves.cpp`, each matching switch case, and cached
  `native-cleanup-dependency-<callee>.json/.pseudocode.txt`; song/loop use
  `native-cleanup-reset-song` and `native-cleanup-reset-loop` instead.
- **A**: `src/guest/native_game_reference.cpp:17–65, 931, 990, 1053, 1183–1233,
  1242, 1264`; header:182. This is PE-backed survey composition, not a live
  Unicorn override that automatically resynchronizes arbitrary guest writes.
- **P**: `src/player/sonic_player.hpp:61`, `.cpp:1547–1553`, Sonic unit/probe.
- **B**: exact original bytes read with `tools/lib/pe_image.py`, pinned EXE SHA
  `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
  Reset `[40bf40,40c023)` SHA `b7a107484cc7753a0f24258c28e55e24a52238497585f6adde4d229c82181dfe`.
  Matches existing report range hash. Bytes prove dword zero at `40bf56`, word
  zero at `40bf5c`, and ordered load/free/store pairs below. This is byte inspection,
  not original-code execution. A standalone cached Reset body dossier was not found;
  `inv-callees-40bf40.json` proves the 19 direct callees, not their runtime behavior.

## Call sequence, signatures, and recovered implementations

SDK names/signatures below come from Main symbols and the pinned headers named
in mapping. `New` declarations win names, but not contradicted access widths.
Each row is in execution order; sites/arguments are R+B and unchanged judge evidence.
`L` means recovered legacy leaf, not permission to replace it with a STUB.

| Site / entry | SDK signature; proposed ordinary call | Existing implementation / limitation |
| --- | --- | --- |
| `40bf42 / 40cd10` | `void voicevolresume()`; `resume_voice_volume` | C helper and L copy saved volume into current volume |
| `40bf49 / 401410` | `void Set_D3DCMPFUNC_Mode(int)`; `set_depth_compare_mode(1)` | L stores `8928c8`; native Display already provides operation |
| `40bf51 / 785330` | `Sint32 pdVibMxStop(Uint32)`; `stop_vibration(0)` | Original L exists; **profile uses Input replacement**, A `input_mod_vibration_stop` |
| stores | clear late-frame word, then editor word | R+C+B, after three successful calls |
| `40bf63 / 403e60` | `void SetQueueDrawingState_BlankScreen()`; `set_blank_queue_mode()` | L sets `3ab98ac=2`, native Display operation |
| `40bf68 / 4257c0` | no SDK signature; cached `void()`; `reset_song_state()` | C TrackState helper and L; do not use `FUN_` identifier |
| `40bf6d / 4257e0` | `void ResetCurrentVoice()`; `reset_current_voice()` | C TrackState helper and L |
| `40bf72 / 424460` | `void dsStop_all()`; `stop_all_sounds()` | C AudioQueueView helper and L; implemented helper returns 0, caller ignores result |
| `40bf79 / 410100` | `void StopPCM(int ch)`; `audio::stop_pcm(-1)` | Native complete channel cleanup exists; L only audio-uninitialized return |
| `40bf7f,40bf86 / 40cff0` | `void WMPClose(int)`; `audio::close_media(0/1)` | Native MediaState exists; L reloads player identity between nested calls |
| `40bf8b / 4208e0` | `void SetupSomeScreenStuff()`; `setup_screen()` | `game_view` + A composition and native Display; not wholly unrecovered |
| `40bf91 / 40d490` | `void MSG_Disp(MSGC*)`; `display_message(nullptr)` | C MessageState helper and L null branch; avoid inventing message rendering |
| `40bf99 / 40b1a0` | SDK `int InitHeap()`; cached `void`; `initialize_heap()` | TaskWorkArena + A reset algorithm; return unused, ownership/reuse needs review |
| `40bf9e / 40b460` | `void InitMasterObjectArray()`; `initialize_tasks()` | TaskScheduler pool + A composition; not a missing behavior STUB |
| `40bfa6 / 4228f0`, 0..9 | `void RunLevelDestructor(int)`; `run_level_destructor(level)` | C LevelState helper and L; dynamic callbacks remain separate recoveries |
| `40bfb4 / 421fb0` | no SDK signature; cached `void()`; `reset_loop_state()` | C LoopState helper and L; nested `504ed0` save-list clear needs identity mapping |
| `40bfc3 / 780fb0`, conditional | `void syFree(void*)`; `release_buffer(first)` | GuestHeap/HLE heap bridge exists; native Session release is loud failure |
| `40bfdb / 780fb0`, conditional | same, second buffer | Distinct identity and later load; do not coalesce equal values |
| `40bfeb,40bff5,40bfff,40c009 / 51f680` | SDK `int nullsub()` | Verified RET, ignored return; source pushes -1, 0xff12, 3abdd00 twice; no native STUB needed |
| `40c00e / 411af0` | `void ___njFogEnable()`; `enable_fog()` | Recovered `engine/ninja_fog`, A adapter, native Display |
| `40c018 / 6443b5` | no SDK signature; CRT `srand(unsigned)` from implementation/bytes | `cpu/crt_rand.cpp`, C final store; name seed constant `0xdead0cab` |

Nested dependencies are not new top-level reset fields. L and cached dossiers show:
`ReleaseSetFile()/ReleaseCamFile()` (`422440/4224b0`, SDK void), native C helpers;
`LevelDestructor_MissionMode()` (`592370`, SDK void), genuine named C STUB;
`504ed0`, SDK unknown, native save-list free counterpart requires binding proof;
`411760/411720`, SDK unknown, cached stream stop/reset bodies and native
`StreamPlayer::stop/reset` exist. `operator_new` (`6443e7`, allocates 0x50 in the
stream initializer), `64fc70` and `51a740` (SDK unknown) are task/audio dependency
boundaries, not permission to stub their already recovered outer behavior.
`syFree` dossier `bridge-heap-780fb0` calls `HeapFree(389d628,0,p)` only if nonnull.
`game_view`'s projection/FOV/render helpers already have recovered implementations;
its nested seam coverage must be retained when binding it.

Profile: `tools/gate/profile_patches.json` has no write intersecting Reset itself.
The active Input jump at `785330` replaces the original rumble function. The loader
rumble replacement there and StopPCM replacement at `410100` are inactive. Original
composed reset judge executes original `785330`, so it does **not** qualify profile
rumble. A routes the replacement explicitly; native Session injects stop_vibration.
No claim of a fresh complete profile checker run.

## Canonical fields, aliases and source users

| Source fact | Current owners/consumers | Required disposition |
| --- | --- | --- |
| `3abdf80`, 4 bytes, load-image virtual zero | GameLoopState owns optional; GameResetState borrows reference; A construction/defaults/export; guest loop reads at `40cbf2` before `4326f0`; game-loop unit/probe mutate it | One plain live late-frame/message-work word; reset and loop borrow it. Native Session currently has **both** `reset_status_` (reset) and `text_window_pending_` (complete_menu_messages). Reconcile these, not another cache. Name's wider semantics remain provisional. |
| `3b29d08`, reset writes 2 bytes, virtual zero | GameResetState owns optional uint16; SonicPlayerGlobals has pointer-to-const-optional; Sonic combines signed16 word with `3b29d0c`, forcing action 87 if nonzero (P); fixtures own separate test instances | One uint16 editor word, consumed with explicit signed16 conversion. SDK New `Bool ssEditorStatus` is **Sint32** (`ninja.h:63`), old header DebugMode is int16. Preserve source word store and adjacent upper bytes, not a 32-bit Bool normalization. Native Session editor_status_ is separate world counterpart. |
| `3b290d0`, 4 bytes, virtual zero | GameResetState optional heap identity; Session temporary_buffer_; R release; cached MessageWindow_Display `421920` also releases and zeroes it after FreeTask/NH_MSG_Close | Buffer role is message-window-related, allocator/creator not established. Keep one native handle and reference identity map; do not equate it with task arena allocation. |
| `3b0e330`, 4 bytes, virtual zero | GameResetState optional heap identity; Session secondary_buffer_; R release | Raw operands also occur near MSG_Close (`40d474`) and MSG_Open (`40e4e6`). Typed creation/publication and aliases not proved by available cached bodies. Do not name it “subtitle buffer” as established fact. |
| `2bcbdf8`, 4 bytes, file-backed **1** | CrtRandState owner; reset borrows RNG; native bootstrap random_seed counterpart and other RNG users | R only writes seed at end, never requires old seed; preserve other RNG consumers on owner migration. |

Source search closure for the **C++ GameResetState type** is R, A/header, unit
`game_reset`, probes `native_game_reset` and `native_game_reset_composed`; Sonic
borrows the field type but does not include/use GameResetState. No production
Session uses GameResetState. No binding of A's `sonic_globals_.forced_at_3b29d08`
to `reset_->word_at_3b29d08` was found: documented borrowing is a contract, not
proof live composition currently wires it. Sonic unit/probe supply their own word.

The raw .text operand scan is in mapping, **not decoded xrefs**: 9 operands for
`3abdf80`, 62 for `3b29d08`, 10 for first buffer, 4 for second. It includes source
users outside recovered maintained consumers; absence of maintained writes cannot
prove no source producer. Cached menu-gate `414df0` reads `3abdf80`; raw operands
also point into `414e2f/414efa/414f19/414f87`, `459560`, `459870` neighborhoods.
First-buffer operands include `421a88,421a9e,421aff,421b2f,421bf0,421c06` in addition
to Reset and MessageWindow_Display. Nearest symbols suggest `421ab0/421bd0` menu
functions, not proven allocation signatures. These need cached/full dossiers before
claiming whole source allocation closure. Do not launch providers from this packet.

Callers: startup `40c030` invokes Reset; GameModeHandler's ActionRestart,
ActionReturn, Adventure, AdventureReturn, Movie, TrialCancel, MissionReturn,
Backdrop/ClearToTitle and CancelTitle paths invoke Reset. Exact sites are in
`src/guest/game_mode_reference.cpp` provenance table and `reset_game` binding:442.
Native flow reaches Session::reset_game; A routes startup:1242 and mode:1264 to its
own legacy reset lambda. None of these paths authorizes retaining two algorithms.

## Ordering, callbacks and missing facts

- First three calls can fail before either zero store. Reset writes both words
  without reading old values; do not eagerly reject missing old words. After that,
  each failing call preserves the entire preceding store/call prefix (R, C, judges).
- Destructor loop runs all ten modes; don't reduce it because callbacks often clear
  their slot. L/C `RunLevelDestructor(0)` releases six SET then six CAM buffers;
  SET count/entry pointers are cleared before frees. Mode 5 skips only when the
  current callback is MissionMode. Callback identity is read after releases; clear
  the callback only after successful return. Callback can replace owners or throw.
- First reset buffer is loaded only after reset_loop_state (`40bfb9`); second only
  after first free/zero (`40bfd1`). Each is cleared **after** successful free. A free
  callback can mutate the second owner: retain live references and re-read it.
  Never snapshot both at entry, clear before call, deduplicate equal identities,
  assume malloc/free compatibility, or destroy all buffers transactionally.
- `WMPClose` reloads published player between stop and reset; callback may replace
  or remove it. audio_playback.cpp implements that re-read. PCM deletes active and
  pending objects before clearing published slots, with active critical section;
  36 record timer/flag stores precede per-channel StopPCM, positional task clear
  follows success. L stops at `41010d` if initialized: preserve that old boundary
  for old fixture behavior, without downgrading recovered native audio to STUB.
- InitHeap's cached source reuses its existing allocation when nonnull and fills
  16 MiB; A instead constructs a new TaskWorkArena. InitMasterObjectArray calls
  cleanup before allocating/initializing 4096 slots. A replaces scheduler after
  clearing work pointers. Identity/borrow invalidation needs explicit reconciliation,
  especially because level destructors run **after** those resets. No blanket proof
  that outstanding task handles or callback contexts survive exists here.
- Loop reset snapshots list presence before zeroing `3b11190` and `3b1118c`; then
  conditionally clears the list (C/L). Cached assembly, not decompiler textual order,
  is authoritative. Missing list fact must fail before stores at the old boundary.
- Missing heap1 fact stops at `40bfb9`; heap2 at `40bfd1`. Failed free retains its
  owner and skips later fog/reseed. R retains pending call and nested boundary;
  missing invoke/cancelled/failed/unavailable outcomes must remain distinguishable.
  Probes don't cover every outcome. Copy-in/copy-out at function exit would lose
  callback visibility and early-failure partial stores.

## Unchanged judges, receipts, and gaps

| Paths / target | Existing evidence and exact scope |
| --- | --- |
| `tests/unit/game_reset.cpp`, manifest `game-reset` | RNG unit checks, one normal free, one missing first heap fact; not owner-composition coverage |
| `tests/probes/native_game_reset_probe.cpp`; `tools/oracles/verify_original_game_reset.py`; `oracle-game-reset` | Export **sadx_native_game_reset**, six uint32 inputs, >=128 uint32 output; deterministic 300 cases, seed `40bf40`; call-N failures and call/site/arg/store/seed trace |
| `tests/probes/native_game_reset_composed_probe.cpp`; `tools/oracles/verify_original_game_reset_composed.py`; `oracle-game-reset-composed` | Export **sadx_native_game_reset_composed**, 14*4096 bytes, fail index, output pages and >=200 words; 300 cases, seed `40bf41`, added-page RNG `40bf42`; exact pages/traces/failure prefixes |
| `tests/unit/game_cleanup.cpp`, manifest `game-cleanup`; `tests/probes/native_game_cleanup_probe.cpp`; `tools/oracles/verify_original_game_cleanup.py`; `oracle-game-cleanup` | Native readable Reset exports the **same old reset ABI in a separate dylib** and runs unchanged reset judge. Other helper/load-save coverage; tests verify task/SET/CAM release order and mission seam |
| `tests/unit/game_loop.cpp`, `native_game_loop_probe.cpp`, `verify_original_game_loop.py`; Sonic counterparts `sonic_player` | Dependent word readers/fixture inputs remain protected; relevant changed ownership needs these unchanged judges, not just reset's two |

Historical tracked `reference/oracle-reports/game_reset.json` and
`game_reset_composed.json` each record 300 cases, 0 mismatches. Composed outcomes
include 50 `completed0:fixtures0` audio-boundary cases; these are matched bounded
outcomes, not 300 completed real resets. `game_cleanup.json` records 2604 cases,
0 mismatches including its nested unchanged 300-case reset report. These are
**old receipts**, not current baseline claims; mapping fingerprints them. Prior
probe missing-page defect/fix evidence is
`reference/production-integration-20261016/journeys-164-evidence.md`.
No Main `build/oracles/{game_reset,game_reset_composed,game_cleanup}.json` was present
at inspection; do not infer anything about Root's concurrent render gate.

No reset-specific frozen corpus, mutant specification/results, or held-out reset
cases were found in tracked reference evidence or the inspected Main paths.
The reset scripts expose no seed/holdout CLI. Cleanup's `run(..., holdout=False)`
is not proof of a held-out run; main uses its default. `tools/gate/mutate.py` is
only generic machinery, not a qualified reset mutant set. Root must establish
baseline and missing qualification before a behavior migration. Do not call these
receipts “unchanged mutants/holdout passed.”

Gaps: callbacks changing words/buffer2/player identity; same-buffer double-free;
real allocator identity/creator and stale task borrows; Sonic live alias; all
missing-state/owner/cancellation branches; profile rumble; initialized audio in
legacy composition; composed view/task/fog bodies (fixtures there); and native
closure of legacy reset (`game_reset.cpp` is not in native-sources.txt, cleanup is).
Old missing configuration-page bug must remain fixed: composed probe always reads
`3a7cf3c` and `38f6ec0`, even when runtime branches never use those facts.

## Proposed interface, adapter and bounded next step

Use **one** `cleanup::reset` implementation as the starting native interface.
Its State should remain a non-owning view of canonical owners (late-frame word,
editor word, two real nullable resource handles, RNG seed), never a second cache.
Replace its 17-field generic Services bundle incrementally with ordinary named
functions taking the relevant system owner by reference: volume/TrackState,
AudioQueueView/PcmState/MediaState, render depth/queue/view/fog, task arena/scheduler,
LevelState/LoopState, and allocator. Do not inject a Session god-object or build a
new dispatch registry. Keep exact sequence in one readable function. Every field,
call and guard is accounted for above; broader names/layouts remain unknown.

Genuine missing definitions get individual named STUB declarations, e.g. existing
mission_level_destructor. Dynamic level callbacks remain typed native callbacks;
unknown callback recovery fails explicitly. Native release_buffer's missing binding
is a resource-boundary failure, not a license to stub recovered `syFree` semantics.
Recovered projection/audio/task/reset operations must use shared implementations.

A narrow `guest/game_reset_reference` boundary should retain old entry/result API,
site/callee tracing, fixture availability, foreign resource identities and old
failure taxonomy. Named bindings must publish/re-read **canonical** words at actual
access/call boundaries and translate exceptions/failures without rollback. Keep
unknown facts lazy (a narrow internal access boundary may be necessary, as in fog),
not optional fields in native State. Adapter must not retain the reset algorithm or
leaf decisions, nor turn native completed audio into an unrecovered call. Preserve
old legacy leaf judge's intentional `41010d` boundary as compatibility policy in the
reference fixture binding, explicitly separate from native audio functionality.

**Prerequisite:** establish canonical frame/message-work and editor ownership across
GameLoopState, reset, Sonic and native Session, with live alias/lifetime tests and
allocator publication context for both buffers. Updating those scalar consumers
and borrowing signatures does not require rewriting every Sonic action or the
whole game loop; it does require examining all readers, making fixture diagnostics
reference-only, and reconciling native reset_status_/text_window_pending_. Leave
Sonic/GameLoop's larger legacy debt explicitly open. Move an owner whole with all
consumers or defer it; don't add optional mirrors to claim closure.

**Bounded connected alternative:** consolidate only the duplicate Reset
orchestration through the existing readable cleanup implementation, preserving
both old probe exports and every diagnostic prefix via named reference bindings.
Keep legacy owner types at the reference boundary until their connected ownership
step is ready. This removes one real duplicate algorithm but is explicitly **not**
a completed reset subsystem/owner migration. If lazy unknowns cannot be preserved
without duplicating decisions, defer that consolidation and take the inventory's
input/frame-policy ownership prerequisite instead; do not relocate R into guest.

Probe sources, unit registration, mutation locations and protected manifests may
need migration. Any required judge/probe rebind is a **separate evidenced judge
commit** under JUDGE.md; preserve export signatures, all input bytes/failure indices,
expected outputs and historical outcome identities. No expectation/tolerance/input
relaxation, new failure masking, or claimed final holdout is authorized here.
