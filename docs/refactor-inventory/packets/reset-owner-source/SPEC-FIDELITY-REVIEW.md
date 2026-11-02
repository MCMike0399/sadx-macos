# Independent spec/fidelity review

Bounded acceptance: no actionable fidelity defect found in the frozen recovered reset/leaf policy or reviewed bindings. This is not system admission, whole Session/loop/Sonic conformance, or approval of the final reserved protocol.

Reviewed head: `e8984111fc218cfcf2d57bc0471c1db6ab75a1dc`; production Source: `744d313ed6a7f02c7c3c5b47ecb78078dca592aa`; immutable base: `6255cc6cabcf14adb32867ea011f7491dbbddd2d`. Reviewed the base-to-head diff and all 19 commits. Specification: PROMPT-complete-refactor §3.3–4, ADR-0009 §4–5, reset-owner CONTEXT/SETUP/BASELINE and the explicit Source packet; STYLE §12 and ARCHITECTURE constrain the review.

Source disposition: the shared reset and every maintained leaf preserve reached reads, call/store ordering, two releases for aliases, later-slot/player/callback reacquisition, clear-after-success, full pending payloads and failure prefixes. Seed publication requires no old seed. Plain frame/editor owners and private knowledge retain forced-before-editor reads even when forced is nonzero, signed16 widening and adjacent-byte preservation. Actual Mac cold-start creation shares policy while retaining constructor/reinitialization cuts. Arena replacement and auxiliary clearing remain unchanged. The new NativeGame-to-Sonic composition is bounded; its separate unforced translation unit avoids the fixture macro layout mismatch.

Binding disposition: all 53 Source seals, 55 review members, 661 immutable-old source hashes, nine oracle reports and 21 qualification artifact hashes match. Independently decoded complete old/current native and original streams: loop 688, Sonic 28,800, stream 600, WinMain 3,000, sound 1,956; zero defined differences. Every scalar/buffer and original observation remains compared. All 1,956 undefined void-return register differences remain raw. Independent fixed nonempty domains, ordered input digests, exclusive destinations and Python comprehension filtering are adequate for these default callers.

Actually ran seven fake-only capture tests, hash checks, static fault reconstruction and read-only stream decoding. Inspected retained execution/build receipts for 13 programs and ten compiling faults; each fault has an observable kill, not a setup failure. No native/original execution, loads, builds, mutations, reserved inputs, network, regeneration or Main writes ran in this review.

Limits remain: retrospective original recovery; one overwritten early build log; missing 53 corpora; Sonic 2,840/2,172 and Title 270 FP partials; Task207 source FAIL; initialized-reference StopPCM41010d; native nonnull release, mission, allocator/reuse and replacement-lifetime gaps. Root owns reserved-protocol adequacy/execution, source-only integration, journeys, protection reconciliation and gate.

## File disposition appendix

Each row explicitly dispositions the complete new/changed helper or test bodies in that review member. Unchanged large consumers were inspected for reset closure; these rows grant no whole-module conformity. Acceptance below has the limits stated above and is independent of prior Source approval.

| Review member | Spec/fidelity disposition |
|---|---|
| `Makefile` | Accept binding recipes: reference linkage, forced Sonic fixture and separate unforced dependency TU. |
| `src/engine/reset_control.hpp` | Accept all owner/view bodies: u32/u16 values, signed widening and mandatory reached observation. |
| `src/game/audio_playback.cpp` | Accept close_media and local Access bodies: missing-player prefix and reacquisition preserved. |
| `src/game/game_cleanup.cpp` | Accept all changed native access/helper bodies: live borrows and shared policy; unchanged helpers retain scope. |
| `src/game/game_cleanup.hpp` | Accept borrowed reset-field migration; native service contract retained. |
| `src/game/game_cleanup_internal.hpp` | Accept every new template/helper: reset, sounds, loop, SET/CAM, level, media close/init, volume, setters, message, vibration and literal format. |
| `src/game/game_loop.hpp` | Accept reference-state declaration separation; no loop rewrite claimed. |
| `src/game/game_reset.hpp` | Accept compatibility declarations; recovered implementation shared, transport moved. |
| `src/game/game_reset_leaves.hpp` | Accept compatibility declarations; leaf algorithms shared, transport moved. |
| `src/guest/game_loop.cpp` | Accept frame fact read migration; inspect existing loop/completion closure without whole-loop approval. |
| `src/guest/game_loop_reference.hpp` | Accept relocated state declaration with canonical frame fact. |
| `src/guest/game_mode_reference.hpp` | Accept reference include/borrow closure. |
| `src/guest/game_reset_leaves_reference.cpp` | Accept every new access/routing/debug body; reference sites, lazy knowledge, constructor and audio cuts preserved. |
| `src/guest/game_reset_leaves_reference.hpp` | Accept state/token and call/result declarations; no payload ownership claim. |
| `src/guest/game_reset_reference.cpp` | Accept every access/operation/observed-access/run body; pending publication and all failure outcomes retained. |
| `src/guest/game_reset_reference.hpp` | Accept constructor and borrowed canonical owners; RNG remains existing CRT owner. |
| `src/guest/music_state_reference.hpp` | Accept migrated leaf include; established shared track owner preserved. |
| `src/guest/native_game_reference.cpp` | Accept init/import/export/default edits and actual editor binding; inspect reset/startup/sound/task closure only. |
| `src/guest/native_game_reference.hpp` | Accept includes and bounded fixture friendship; actual production layout retained. |
| `src/guest/reset_buffer_tokens.hpp` | Accept all token methods: real stable map objects, null and foreign-token rejection; no allocator claim. |
| `src/guest/reset_control_reference.hpp` | Accept all sidecar constructors/operators/import/reset/borrow bodies: absence cannot expose stale values. |
| `src/platform/mac/mac_audio_playback.mm` | Accept complete create_media_players/Access bodies and inspect backend constructors; actual native cold-start support retained. |
| `src/player/sonic_player.cpp` | Accept complete changed run_sonic_player body within editor migration; forced read and failure-before-action preserved. |
| `src/player/sonic_player.hpp` | Accept editor-view member replacement; remaining Sonic state is not certified. |
| `src/runtime/native_file_select_display.cpp` | Accept complete_menu_messages body: same reached nonzero failure on canonical owner. |
| `src/runtime/native_session.cpp` | Accept reset_game body/borrows; inspect services, replacement and nonnull release boundaries, no whole Session approval. |
| `src/runtime/native_session.hpp` | Accept canonical frame/editor fields; zero-only source identity consolidation explicitly bounded. |
| `tests/fixtures/reset_composition_fixture.hpp` | Accept complete fixture interface/destructor contract; hides NativeGame from forced TU. |
| `tests/probes/native_frame_completion_probe.cpp` | Accept complete changed probe body: canonical field/include/token transport; defined ABI, extents and output coverage retained. |
| `tests/probes/native_game_loop_probe.cpp` | Accept complete changed probe body: canonical field/include/token transport; defined ABI, extents and output coverage retained. |
| `tests/probes/native_game_reset_composed_probe.cpp` | Accept complete changed probe body: canonical field/include/token transport; defined ABI, extents and output coverage retained. |
| `tests/probes/native_game_reset_probe.cpp` | Accept complete changed probe body: canonical field/include/token transport; defined ABI, extents and output coverage retained. |
| `tests/probes/native_sonic_player_probe.cpp` | Accept complete changed probe body: canonical field/include/token transport; defined ABI, extents and output coverage retained. |
| `tests/probes/native_stream_players_probe.cpp` | Accept complete changed probe body: canonical field/include/token transport; defined ABI, extents and output coverage retained. |
| `tests/probes/reset_composition_fixture.cpp` | Accept every fixture/Impl body; actual NativeGame constructed unforced and live owners borrowed. |
| `tests/unit/game_loop.cpp` | Accept complete changed fixture/test bodies: reference includes and canonical borrows retain existing assertions and domains. |
| `tests/unit/game_reset.cpp` | Accept complete changed fixture/test bodies: reference includes and canonical borrows retain existing assertions and domains. |
| `tests/unit/music_state_reference.cpp` | Accept complete changed fixture/test bodies: reference includes and canonical borrows retain existing assertions and domains. |
| `tests/unit/native_logo_scene.cpp` | Accept complete added replacement/observer/callback bodies; held handles never dereference expired arena. |
| `tests/unit/reset_native_contract.cpp` | Accept complete main/helper bodies: failure positions, aliases, replacement, validation and foreign tokens. |
| `tests/unit/reset_order_characterization.cpp` | Accept complete fixture/emit/main bodies; immutable 55-row characterization independently matches. |
| `tests/unit/save_frame_characterization.cpp` | Accept complete changed fixture/test bodies: reference includes and canonical borrows retain existing assertions and domains. |
| `tests/unit/sonic_player.cpp` | Accept changed fixture and complete added composition/observation lambdas; test layout correction and bounded real reader. |
| `tools/oracles/verify_original_game_cleanup.py` | Accept complete changed main/report body: dependency lineage expanded; original cases, comparisons, stops and verdict logic unchanged. |
| `tools/oracles/verify_original_game_loop.py` | Accept complete changed main/report body: dependency lineage expanded; original cases, comparisons, stops and verdict logic unchanged. |
| `tools/oracles/verify_original_game_reset.py` | Accept complete changed main/report body: dependency lineage expanded; original cases, comparisons, stops and verdict logic unchanged. |
| `tools/oracles/verify_original_game_reset_composed.py` | Accept complete changed main/report body: dependency lineage expanded; original cases, comparisons, stops and verdict logic unchanged. |
| `tools/oracles/verify_original_sonic_player.py` | Accept complete changed main/report body: dependency lineage expanded; original cases, comparisons, stops and verdict logic unchanged. |
| `tools/oracles/verify_original_stream_players.py` | Accept complete changed main/report body: dependency lineage expanded; original cases, comparisons, stops and verdict logic unchanged. |
| `reference/packets/reset-owner-source/test_capture_consumers.py` | Accept every fake/parser/test body; seven tests actually passed here, without native loads. |
| `reference/packets/reset-owner-source/run_reset_faults.py` | Accept complete mutation runner: exclusive evidence, compile-error rejection, assertion evidence and finally restoration; ten retained semantic kills checked statically. |
| `reference/packets/reset-owner-source/capture_consumers.py` | Accept every function/class body for finite defaults: scalar/bytes/extents, trace filtering, exclusive output and nonempty ordered-domain validation. |
| `tests/unit/reset_leaf_contract.cpp` | Accept complete fixture/output/main bodies; 15 old/current rows preserve completed-survey unknowns and leaf limits. |
| `tests/unit/reset_pending_contract.cpp` | Accept complete main body: 113 rows retain optional argument presence/payload and four owner outcomes. |
| `tests/probes/native_game_cleanup_probe.cpp` | Accept complete changed probe body: canonical field/include/token transport; defined ABI, extents and output coverage retained. |

## Evidence interpretation appendix

Evidence root (read only): `/Users/burbujamc/.t3/worktrees/sadx-macos/t3-6a2279e4`.

- Decoded `CAPTURE-CONTRACTS.json` old streams and `build/reset-current-qualified/<slug>/<slug>.{abi,original}.gz` completely, rejecting truncated records, checking sequential ordinals, argument byte lengths, ordered input/export hashes and fixed counts. Old missing element-type labels were supplied from the frozen declarations; missing original case labels were only invocation ordinals. Only the sound export's undefined return register was normalized; full raw files remain unchanged. Full original records preserve every inherited per-case mismatch identity, not merely totals.
- `build/reset-complete-consumer-comparison-typed.json` agrees with the independent decoding above. `RECOVERY-BINDING.json` artifact hashes match. `build/reset-old` and immutable git both match all 661 frozen source hashes. WinMain/stream original recovery remains retrospective, not a before-edit execution receipt. The failed initial captures, comprehension duplicates and untyped comparison remain failed evidence.
- The 55-line frozen order matrix equals the retained qualified output byte for byte. The supplementary pending and leaf outputs equal immutable-old outputs byte for byte (113 and 15 physical lines). Source inspection confirms the composed probe stores the editor as u16 and retains upper neighboring bytes; existing 300-case composed receipt preserves its 50 initialized-audio boundary cases.
- Inspected all ten `build/reset-faults-744d313/<fault>/` build logs, matrix outputs, original reports and native assertion outputs. Reconstructed each fault only in memory and matched its retained source digest. Every build exits zero; nine have a changed successful matrix, the remaining vibration fault has 72 original mismatches. Assertion aborts occur only where retained; no compile failure or arbitrary crash was counted as proof. Restoration receipt exits zero. The runner was not executed here.
- Inspected the 13 qualified program entries, corresponding logs, corrected Sonic/logo receipts, actual Mac playback test and unforced fixture recipe/dependencies. Initial Sonic/logo aborts remain distinct failed attempts. These are retained runtime receipts, not executions performed by this reviewer. Native force-load/closure logs establish their recorded link boundary, not route or resource-lifetime correctness.
- Read the six changed oracle bodies and original default consumer callers for domain/ABI closure; changed report maps add dependencies rather than alter judges. No original/native case generator was executed. No reserved value was used. Neither FINAL-UNRUN nor the system's later admission checks receives automatic acceptance from this review.
